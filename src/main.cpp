#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <ESP32Encoder.h>
#include <Preferences.h>
#include "LatheParameter.h"
#include "JogMode.h"
#include "Setting.h"
#include "config.h"
#include "handler/DisplayHandler.h"
#include "handler/ButtonHandler.h"

TaskHandle_t userInterfaceTask;

Preferences preferences;
LatheParameter* latheParameter;
DisplayHandler* displayHandler;
ButtonHandler* buttonHandler;

// RPM Encoder
ESP32Encoder encoder;
int64_t encoderLastUpm = 0;     // letzte Position des Encoders (UPM) in Steps *4 (Quad)
int64_t encoderAct = 0;      // aktue Position des Encoders in Steps *4 (Quad)
int64_t encoderLastSteps = 0; // Tempvariable für encoderDoSteps
float encoderLastStepsFrac = 0.0f;

// RPM
int rpmMillisTemp = 0; // Tempspeicher für millis

// Lathe parameters
float stepperStepsPerEncoderSteps = 0.0f; // Leitspindel Schrittmotor Schritte pro Drehschritt (errechnet sich aus stepper Steps, threadPitch und spindleMmPerRound)
unsigned long lastStepTime = 0;
bool stepPinIsOn = false;
bool stepDelayDirection = false; // To reset stepDelayUs when direction changes.
bool directionChanged = false;

// Degree calculation
float encoderDeg = 0; // Winkel des encoders;

// #################################################################################
// ### 
// ### Second core task - Userinterface
// ### 
// #################################################################################

void secondCoreTask( void * parameter) {
  displayHandler = new DisplayHandler();
  buttonHandler = new ButtonHandler(BUTTON_ADD_PIN, BUTTON_REMOVE_PIN, BUTTON_TARGET_PIN, BUTTON_POSITION_PIN, BUTTON_JOG_LEFT_PIN, BUTTON_JOG_RIGHT_PIN);

  for(;;) {
    buttonHandler->handleButtons(latheParameter);
    displayHandler->updateDisplay(latheParameter);
    delay(20);
  }
}

// #################################################################################
// ### 
// ### Primary core task - stepper driver
// ### 
// #################################################################################

void updatePosition(bool direction) {
  float encoderStepsPerStepperStep = (ENCODER_PULS_PER_REVOLUTION * 4) / ((latheParameter->spindlePitch().metricFeed / (SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO / MICROSTEPS_PER_REVOLUTION)));
  float encoderStepsPerStepperStep_frac = encoderStepsPerStepperStep - (int) encoderStepsPerStepperStep;
  int encoderStepsPerStepperStep_int = encoderStepsPerStepperStep - encoderStepsPerStepperStep_frac;

  if (direction) {
        encoderLastStepsFrac += encoderStepsPerStepperStep_frac;
        encoderLastSteps += encoderStepsPerStepperStep_int + (int)encoderLastStepsFrac;
        encoderLastStepsFrac -= (int)encoderLastStepsFrac;
      } else {
        encoderLastStepsFrac += encoderStepsPerStepperStep_frac;
        encoderLastSteps -= encoderStepsPerStepperStep_int + (int)encoderLastStepsFrac;
        encoderLastStepsFrac -= (int)encoderLastStepsFrac;
      }
}

void endSingleStep() {
  digitalWrite(STEP_PIN, LOW);
}

// Moves the stepper.
bool startSingleStep(bool dir, bool isJog) {
  bool needBacklashCompensation = false;
  // Start slow if direction changed.
  if (stepDelayDirection != dir) {
    // stepDelayUs = PULSE_MAX_US;
    stepDelayDirection = dir;
    digitalWrite(DIR_PIN, dir ? HIGH : LOW);
    delayMicroseconds(10); // Need for changing Direction
    Serial.println("Direction change");
    directionChanged = true;
    needBacklashCompensation = true;
  }

  //Compensate Backlash
  if (needBacklashCompensation) {
    for (int backlashStep = 1; backlashStep <= latheParameter->backlash(); ++backlashStep) {
      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(100);
      digitalWrite(STEP_PIN, LOW);
      delayMicroseconds(400);
    }
  }

  if (!latheParameter->isSpindelInSync() && !isJog) {
    if ((int)encoderDeg <= 5) {
      latheParameter->setSpindelInSync();
      Serial.println("Spindle in sync!");
    } else {
      Serial.println((int)encoderDeg);
      encoderLastSteps = encoder.getCount();
      return false;
    }
  }

 if (!isnan(latheParameter->stepperTarget()) && (abs(latheParameter->stepperTarget() - latheParameter->stepperPosition()) <= THRESHOLD) && !directionChanged && !isJog) {
    Serial.println("Target reached");
    latheParameter->stopSpindel();
    return false; // Target reached!
  }

  // dir true = clockwise
  float offset = dir ? SINGLE_STEP : -SINGLE_STEP;
  latheParameter->setStepperPosition(latheParameter->stepperPosition() + offset);

  updatePosition(dir);
  digitalWrite(STEP_PIN, HIGH);
  directionChanged = false;
  return true;
}

void performBlockingStep(bool direction, float multiplier) {
  startSingleStep(direction, true);
  delayMicroseconds(20000/multiplier);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(20000/multiplier);
}

void setup() {
  Serial.begin(115200);
  latheParameter = new LatheParameter(&preferences);
  ESP32Encoder::useInternalWeakPullResistors = UP;
  encoder.attachFullQuad(ENCODER_A, ENCODER_B);
  encoder.clearCount();

  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);

  stepDelayDirection = digitalRead(DIR_PIN);

  xTaskCreatePinnedToCore(
    secondCoreTask, /* Function to implement the task */
    "UserInterface", /* Name of the task */
    10000,  /* Stack size in words */
    nullptr,  /* Task input parameter */
    2,  /* Priority of the task */
    &userInterfaceTask,  /* Task handle. */
    0
  ); /* Core where the task should run */
}

void loop() {
  // Current encoder position
  encoderAct = encoder.getCount();

  // Stepperschritte pro Drehzahlencoder Schritt berechnen
  stepperStepsPerEncoderSteps = ((latheParameter->spindlePitch().metricFeed / (SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO / MICROSTEPS_PER_REVOLUTION))) / (ENCODER_PULS_PER_REVOLUTION * 4);

  // max RPM calculation
  latheParameter->setMaxRpm(abs((MAX_MOTOR_SPEED * 60) / ((latheParameter->spindlePitch().metricFeed / (SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO)) * 200)));

  // calculate rpm
  if (millis() >= rpmMillisTemp + RPM_MEASUREMENT_TIME_MILLIS) {
    latheParameter->setRpm((abs(encoderAct - encoderLastUpm) * 60000 / (millis() - rpmMillisTemp)) / (ENCODER_PULS_PER_REVOLUTION * 4));
    encoderLastUpm = encoderAct;
    rpmMillisTemp += RPM_MEASUREMENT_TIME_MILLIS;

    // Stop the spindel if we are running to fast.
    if (latheParameter->rpm() > latheParameter->maxRpm()) {
      latheParameter->stopSpindel();
    }
  }

  // calculate degrees
  encoderDeg = float(encoderAct % (ENCODER_PULS_PER_REVOLUTION * 4)) * 360 / (ENCODER_PULS_PER_REVOLUTION * 4);
  if (encoderDeg < 0) {
    encoderDeg = 360 + encoderDeg;
  }

  // Drive the motor
  int encoderActDifference = encoderAct - encoderLastSteps;
  int stepsToDo = encoderActDifference * stepperStepsPerEncoderSteps;

  switch (latheParameter->settingMode()) {
    case SettingModeBacklash:
      startSingleStep(true, true);
      delay(250);
      startSingleStep(false, true);
      delay(250);
      break;
    default:
      break;
  }

  if (latheParameter->isAutoMoveToZero()) {
    // Auto move to zero handler 
    if (abs(latheParameter->stepperPosition()) >= THRESHOLD) {
      bool direction = latheParameter->stepperPosition() > 0 ? false : true;
      performBlockingStep(direction, latheParameter->autoMoveToZeroMultiplier());
      latheParameter->setAutoMoveToZeroMultiplier(min(latheParameter->autoMoveToZeroMultiplier() + AUTO_MOVE_INCREASE_STEP, MAX_SPEED_MULTIPLIER));
    } else {
      latheParameter->setAutoMoveToZeroMultiplier(1.0f);
      latheParameter->setAutoMoveToZero(false);
    }
  } else if (latheParameter->currentJogMode() != neutral) {
    // Jog handler 
    bool direction = latheParameter->currentJogMode() == left ? true : false;
    performBlockingStep(direction, latheParameter->jogCurrentSpeedMultiplier());
    latheParameter->setJogCurrentSpeedMultiplier(min(latheParameter->jogCurrentSpeedMultiplier() + JOG_INCREASE_STEP, MAX_SPEED_MULTIPLIER));
  } else {
    // Spindle step handler
    if (abs(stepsToDo) >= 1 && !stepPinIsOn) {
      bool direction = stepsToDo > 0;

      if (latheParameter->isSpindelEnabled()) {
        stepPinIsOn = startSingleStep(direction, false);
        if (stepPinIsOn) {
          lastStepTime = micros();
        }
      } else {
        encoderLastSteps = encoderAct;
      }
    }
  
    if (micros() - lastStepTime >= MINIMUM_STEP_PULS_WIDTH && stepPinIsOn) {
        endSingleStep();
        stepPinIsOn = false;
    }
  }
}