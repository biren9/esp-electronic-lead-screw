#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <ESP32Encoder.h>
#include "LatheParameter.h"
#include "Button.h"
#include "JogMode.h"
#include "Setting.h"
#include "config.h"
#include "handler/DisplayHandler.h"

ButtonConfig buttonConfigs[4] = {
  ButtonConfig{BUTTON_ADD_PIN, readyToTrigger, 0, 0},
  ButtonConfig{BUTTON_REMOVE_PIN, readyToTrigger, 0, 0},
  ButtonConfig{BUTTON_TARGET_PIN, readyToTrigger, 0, 0},
  ButtonConfig{BUTTON_POSITION_PIN, readyToTrigger, 0, 0}
};

float jogIncreaseStep = 0.05f;
float autoMoveIncreaseStep = 0.2f;

TaskHandle_t userInterfaceTask;

LatheParameter* latheParameter  = new LatheParameter();
DisplayHandler* displayHandler;

// RPM Encoder
ESP32Encoder encoder;
int64_t encoderLastUpm = 0;     // letzte Position des Encoders (UPM) in Steps *4 (Quad)
int64_t encoderAct = 0;      // aktue Position des Encoders in Steps *4 (Quad)
int64_t encoderLastSteps = 0; // Tempvariable für encoderDoSteps
float encoderLastStepsFrac = 0.0f;

// RPM
int rpmMillisMeasure = 500; // Messzeit für UPM in Millisekunden
int rpmMillisTemp = 0; // Tempspeicher für millis

// Lathe parameters
float stepperStepsPerEncoderSteps = 0.0f; // Leitspindel Schrittmotor Schritte pro Drehschritt (errechnet sich aus stepper Steps, threadPitch und spindleMmPerRound)
bool directionChanged = false;
unsigned long lastStepTime = 0;
bool stepPinIsOn = false;
bool stepDelayDirection = false; // To reset stepDelayUs when direction changes.
float stepsToDoLater = 0.0f;

// Degree calculation
int64_t encoderAbs=0;
float encoderDeg = 0; // Winkel des encoders;

// #################################################################################
// ### 
// ### Second core task - Userinterface
// ### 
// #################################################################################

void secondCoreTask( void * parameter) {
  displayHandler = new DisplayHandler();

  pinMode(BUTTON_ADD_PIN, INPUT_PULLUP);
  pinMode(BUTTON_REMOVE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_TARGET_PIN, INPUT_PULLUP);
  pinMode(BUTTON_POSITION_PIN, INPUT_PULLUP);
  pinMode(BUTTON_JOG_LEFT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_JOG_RIGHT_PIN, INPUT_PULLUP);

  for(;;) {

    switch (Button::checkButtonState(&buttonConfigs[BUTTON_ADD_INDEX])) {
    case recognizedLong:
      if (latheParameter->settingMode() == SettingModeNone) {
        latheParameter->stopSpindel();
        latheParameter->setSettingMode(SettingModeSetting);
      } else {
        latheParameter->setSettingMode(Setting::next(latheParameter->settingMode()));
      }
      buttonConfigs[BUTTON_ADD_INDEX].handledAndAcceptMoreLong();
      break;
    case recognizedShort: {
      switch (latheParameter->settingMode()) {
        case SettingModeNone: {
          unsigned int size = latheParameter->availablePitches();
          latheParameter->setFeedIndex(min(latheParameter->feedIndex()+1, size-1));
          break;
        }
        case SettingModeBacklash:
          latheParameter->setBacklash(latheParameter->backlash()+1);
          break;
        case SettingModeMeasurementSystem:
          latheParameter->setMetricFeed(!latheParameter->isMetricFeed());
          latheParameter->setFeedIndex(0);
          break;
        case SettingModeInvertFeed:
          latheParameter->setInvertFeed(!latheParameter->isInvertFeed());
          break;
        default:
          break;
      }
      buttonConfigs[BUTTON_ADD_INDEX].handled();
      break;
    }
    default:
      break;
    }

    switch (Button::checkButtonState(&buttonConfigs[BUTTON_REMOVE_INDEX])) {
    case recognizedLong:
      latheParameter->setSettingMode(SettingModeNone);
      buttonConfigs[BUTTON_REMOVE_INDEX].handled();
      break;
    case recognizedShort:
      switch (latheParameter->settingMode()) {
        case SettingModeNone:
          latheParameter->setFeedIndex(max(latheParameter->feedIndex()-1, 0u));
          break;
        case SettingModeBacklash:
          latheParameter->setBacklash(max(latheParameter->backlash()-1, 0));
          break;
        case SettingModeMeasurementSystem:
          latheParameter->setMetricFeed(!latheParameter->isMetricFeed());
          latheParameter->setFeedIndex(0);
          break;
        case SettingModeInvertFeed:
          latheParameter->setInvertFeed(!latheParameter->isInvertFeed());
          break;
        default:
          break;
      }
      buttonConfigs[BUTTON_REMOVE_INDEX].handled();
      break;
    default:
      break;
    }

    switch (Button::checkButtonState(&buttonConfigs[BUTTON_TARGET_INDEX])) {
    case recognizedLong:
    if (isnan(latheParameter->stepperTarget())) {
        latheParameter->setStepperTarget(latheParameter->stepperPosition());
      } else {
        latheParameter->setStepperTarget(NAN);
      }
      buttonConfigs[BUTTON_TARGET_INDEX].handled();
      break;
    case recognizedShort:
      if (latheParameter->isSpindelEnabled()) {
        latheParameter->stopSpindel();
      } else {
        latheParameter->startSpindel();
      }
      buttonConfigs[BUTTON_TARGET_INDEX].handled();
      break;
    default:
      break;
    }

    switch (Button::checkButtonState(&buttonConfigs[BUTTON_POSITION_INDEX])) {
    case recognizedLong:
      latheParameter->setStepperPosition(0.0f);
      buttonConfigs[BUTTON_POSITION_INDEX].handled();
      
      break;
    case recognizedShort:
      latheParameter->stopSpindel();
      latheParameter->setAutoMoveToZeroMultiplier(1.0f);
      latheParameter->setAutoMoveToZero(latheParameter->isAutoMoveToZero());
      buttonConfigs[BUTTON_POSITION_INDEX].handled();
      break;
    default:
      break;
    }

    int8_t maxCounter = 5;
    if (digitalRead(BUTTON_JOG_LEFT_PIN) == LOW) {
      latheParameter->setJogReadCounter(max(latheParameter->jogReadCounter()-1, -maxCounter));
      if (latheParameter->currentJogMode() != left && latheParameter->jogReadCounter() == -maxCounter) {
        latheParameter->setJogCurrentSpeedMultiplier(1.0f);
        latheParameter->setCurrentJogMode(left);
      }
    
      latheParameter->stopSpindel();
    } else if (digitalRead(BUTTON_JOG_RIGHT_PIN) == LOW) {
      latheParameter->setJogReadCounter(min(latheParameter->jogReadCounter()+1, -maxCounter));
      if (latheParameter->currentJogMode() != right && latheParameter->jogReadCounter() <= maxCounter) {
        latheParameter->setJogCurrentSpeedMultiplier(1.0f);
        latheParameter->setCurrentJogMode(right);
      }
      latheParameter->stopSpindel();
    } else {
      latheParameter->setJogReadCounter(0);
      latheParameter->setJogCurrentSpeedMultiplier(1.0f);
      latheParameter->setCurrentJogMode(neutral);
    }

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

/* Actual this shouldn't be necessary */
  // if (directionChanged && !waitToSyncSpindel && !isJog) {
  //   waitToSyncSpindel = true;
  //   Serial.println("Start syncing spindle...");
  // }

  if (!latheParameter->isSpindelInSync()) {
    if ((int)encoderDeg <= 5) {
      latheParameter->setSpindelInSync();
      Serial.println("Spindle in sync!");
      // Spindel in sync
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
  latheParameter->setStepperPosition(latheParameter->stepperPosition() + dir ? SINGLE_STEP : -SINGLE_STEP);

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
    NULL,  /* Task input parameter */
    2,  /* Priority of the task */
    &userInterfaceTask,  /* Task handle. */
    0
  ); /* Core where the task should run */
}

void loop() {
  // Curent encoder position
  encoderAct = encoder.getCount();

  // Stepperschritte pro Drehzahlencoder Schritt berechnen
  stepperStepsPerEncoderSteps = ((latheParameter->spindlePitch().metricFeed / (SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO / MICROSTEPS_PER_REVOLUTION))) / (ENCODER_PULS_PER_REVOLUTION * 4);

  // max RPM calculation
  latheParameter->setMaxRpm(abs((MAX_MOTOR_SPEED * 60) / ((latheParameter->spindlePitch().metricFeed / (SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO)) * 200)));

  // calculate rpm
  if (millis() >= rpmMillisTemp + rpmMillisMeasure) {
    latheParameter->setRpm((abs(encoderAct - encoderLastUpm) * 60000 / (millis() - rpmMillisTemp)) / (ENCODER_PULS_PER_REVOLUTION * 4));
    encoderLastUpm = encoderAct;
    rpmMillisTemp = rpmMillisTemp + rpmMillisMeasure;

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
      latheParameter->setAutoMoveToZeroMultiplier(min(latheParameter->autoMoveToZeroMultiplier() + autoMoveIncreaseStep, MAX_SPEED_MULTIPLIER));
    } else {
      latheParameter->setAutoMoveToZeroMultiplier(1.0f);
      latheParameter->setAutoMoveToZero(false);
    }
  } else if (latheParameter->currentJogMode() != neutral) {
    // Jog handler 
    bool direction = latheParameter->currentJogMode() == left ? true : false;
    performBlockingStep(direction, latheParameter->jogCurrentSpeedMultiplier());
    latheParameter->setJogCurrentSpeedMultiplier(min(latheParameter->jogCurrentSpeedMultiplier() + jogIncreaseStep, MAX_SPEED_MULTIPLIER));
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