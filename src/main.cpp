#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Encoder.h>
#include <Preferences.h>
#include "Button.h"
#include "JogMode.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     0 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32 0x7A

#define ENCODER_A 14 // Drehzahlencoder Pin A
#define ENCODER_B 27 // Drehzahlencoder Pin B
#define ENCODER_PULS_PER_REVOLUTION 600 // Drehzahlencoder Pin B

#define DIR_PIN 4
#define STEP_PIN 2
#define STEPPER_GEAR_RATIO 0.5f // example: 20 teath on stepper & 40 teeth on spindel is a 1/2 gear ratio
#define MICROSTEPS_PER_REVOLUTION 400
#define SPINDEL_THREAD_PITCH 1.5f // value in mm
#define MINIMUM_STEP_PULS_WIDTH 4 // in µs
#define THRESHOLD 0.001f
#define MAX_MOTOR_SPEED 1200 // Höchstgeschwindigkeit
#define SINGLE_STEP ((SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO) / MICROSTEPS_PER_REVOLUTION)

// Buttons
#define BUTTON_ADD_PIN 23
#define BUTTON_ADD_INDEX 0

#define BUTTON_REMOVE_PIN 18
#define BUTTON_REMOVE_INDEX 1

#define BUTTON_TARGET_PIN 13
#define BUTTON_TARGET_INDEX 2

#define BUTTON_POSITION_PIN 17
#define BUTTON_POSITION_INDEX 3

#define BUTTON_JOG_LEFT_PIN 33
#define BUTTON_JOG_RIGHT_PIN 35

ButtonConfig buttonConfigs[4] = {
  ButtonConfig{BUTTON_ADD_PIN, readyToTrigger, 0, 0},
  ButtonConfig{BUTTON_REMOVE_PIN, readyToTrigger, 0, 0},
  ButtonConfig{BUTTON_TARGET_PIN, readyToTrigger, 0, 0},
  ButtonConfig{BUTTON_POSITION_PIN, readyToTrigger, 0, 0}
};

JOGMode currentJogMode = neutral;
float maxSpeedMultiplier = 100.0f;
float jogCurrentSpeedMultiplier = 1.0f;
float jogIncreaseStep = 0.05f;
float autoMoveToZeroMultiplier = 1.0f;
float autoMoveIncreaseStep = 0.2f;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TaskHandle_t userInterfaceTask;
Preferences preferences;

// RPM Encoder
ESP32Encoder encoder;
int64_t encoderLastUpm = 0;     // letzte Position des Encoders (UPM) in Steps *4 (Quad)
int64_t encoderAct = 0;      // aktue Position des Encoders in Steps *4 (Quad)
int64_t encoderLastSteps = 0; // Tempvariable für encoderDoSteps
float encoderLastStepsFrac = 0.0f;

// RPM
int rpm = 0; // Umdrehung pro Minute
int rpmMillisMeasure = 500; // Messzeit für UPM in Millisekunden
int rpmMillisTemp = 0; // Tempspeicher für millis
int maxRpm = 0; // maximale, fehlerfreie UPM für Drehbank. Wird errechnet aus dem Vorschub

// Lathe parameters
int feedIndex = 10;
float availableFeeds[] = {-3.0f, -2.0f, -1.5f, -1.25f, -1.0f, -0.8f, -0.75f, -0.5f, 0.02f, 0.05f, 0.1f, 0.15f, 0.20f, 0.25f, 0.30f, 0.5f, 0.75f, 0.8f, 1.0f, 1.25f, 1.5f, 2.0f, 3.0f};
float stepperStepsPerEncoderSteps = 0.0f; // Leitspindel Schrittmotor Schritte pro Drehschritt (errechnet sich aus stepper Steps, threadPitch und spindleMmPerRound)
bool directionChanged = false;
float stepperPosition = 0.0f; // in mm
float stepperTarget = NAN; // in mm
bool waitToSyncSpindel = false;
unsigned long lastStepTime = 0;
bool stepPinIsOn = false;
int backlashInSteps = 20;
bool stepDelayDirection = false; // To reset stepDelayUs when direction changes.
float stepsToDoLater = 0.0f;
bool isSpindelEnabled = false;
bool autoMoveToZero = false;
int8_t jogReadCounter = 0;

// Degree calculation
int64_t encoderAbs=0;
float encoderDeg = 0; // Winkel des encoders;

float spindleMmPerRound() {
  return availableFeeds[feedIndex];
}

// #################################################################################
// ### 
// ### Second core task - Userinterface
// ### 
// #################################################################################

void secondCoreTask( void * parameter) {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  preferences.begin("settings", false);
  backlashInSteps = preferences.getInt("backlash", backlashInSteps);
  feedIndex = preferences.getInt("feedIndex", feedIndex);

  pinMode(BUTTON_ADD_PIN, INPUT_PULLUP);
  pinMode(BUTTON_REMOVE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_TARGET_PIN, INPUT_PULLUP);
  pinMode(BUTTON_POSITION_PIN, INPUT_PULLUP);
  pinMode(BUTTON_JOG_LEFT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_JOG_RIGHT_PIN, INPUT_PULLUP);

  for(;;) {

    switch (Button::checkButtonState(&buttonConfigs[BUTTON_ADD_INDEX])) {
    case recognizedLong:
      backlashInSteps += 1;
      preferences.putInt("backlash", backlashInSteps);
      buttonConfigs[BUTTON_ADD_INDEX].handledAndAcceptMoreLong();
      break;
    case recognizedShort: {
      int size = sizeof(availableFeeds) / sizeof(float);
      feedIndex = min(feedIndex+1, size-1);
      preferences.putInt("feedIndex", feedIndex);
      buttonConfigs[BUTTON_ADD_INDEX].handled();
      break;
    }
    default:
      break;
    }

    switch (Button::checkButtonState(&buttonConfigs[BUTTON_REMOVE_INDEX])) {
    case recognizedLong:
      backlashInSteps = max(0, backlashInSteps - 1);
      preferences.putInt("backlash", backlashInSteps);
      buttonConfigs[BUTTON_REMOVE_INDEX].handledAndAcceptMoreLong();
      break;
    case recognizedShort:
      feedIndex = max(feedIndex-1, 0);
      preferences.putInt("feedIndex", feedIndex);
      buttonConfigs[BUTTON_REMOVE_INDEX].handled();
      break;
    default:
      break;
    }

    switch (Button::checkButtonState(&buttonConfigs[BUTTON_TARGET_INDEX])) {
    case recognizedLong:
    if (isnan(stepperTarget)) {
        stepperTarget = stepperPosition;
        waitToSyncSpindel = true;
      } else {
        stepperTarget = NAN;
        waitToSyncSpindel = false;
      }
      buttonConfigs[BUTTON_TARGET_INDEX].handled();
      break;
    case recognizedShort:
      if (isSpindelEnabled) {
        stopSpindel();
      } else {
        startSpindel();
      }
      buttonConfigs[BUTTON_TARGET_INDEX].handled();
      break;
    default:
      break;
    }

    switch (Button::checkButtonState(&buttonConfigs[BUTTON_POSITION_INDEX])) {
    case recognizedLong:
      stepperTarget -= stepperPosition; // TODO: Evaluate
      stepperPosition = 0.0f;
      buttonConfigs[BUTTON_POSITION_INDEX].handled();
      
      break;
    case recognizedShort:
      stopSpindel();
      autoMoveToZeroMultiplier = 1.0f;
      autoMoveToZero = !autoMoveToZero;
      buttonConfigs[BUTTON_POSITION_INDEX].handled();
      break;
    default:
      break;
    }

    int8_t maxCounter = 5;
    if (digitalRead(BUTTON_JOG_LEFT_PIN) == LOW) {
      jogReadCounter = max(jogReadCounter-1, -maxCounter);
      if (currentJogMode != left && jogReadCounter == -maxCounter) {
        jogCurrentSpeedMultiplier = 1.0f;
        currentJogMode = left;
      }
    
      stopSpindel();
    } else if (digitalRead(BUTTON_JOG_RIGHT_PIN) == LOW) {
      jogReadCounter = min(jogReadCounter+1, -maxCounter);
      if (currentJogMode != right && jogReadCounter <= maxCounter) {
        jogCurrentSpeedMultiplier = 1.0f;
        currentJogMode = right;
      }
      stopSpindel();
    } else {
      jogReadCounter = 0;
      currentJogMode = neutral;
      jogCurrentSpeedMultiplier = 1.0f;
    }
  
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("RPM " + String(rpm));
    display.setTextSize(1);
    display.println("Max " + String(maxRpm));

    String spindelState;
    if (isSpindelEnabled) {
      if (waitToSyncSpindel) {
        spindelState = "Syncing";
      } else {
        spindelState = "On";
      }
    } else {
      spindelState = "Off";
    }

    display.println("Feed " + String(spindleMmPerRound()) + " " + spindelState);
    display.println("Degree " + String(encoderDeg));
    if (!isnan(stepperTarget)) {
      display.println("Target " + String(stepperTarget));
    }
    display.println("Position " + String(stepperPosition));
    display.println("Backlash " + String(backlashInSteps));
    display.display();
    delay(20);
  }
}

// #################################################################################
// ### 
// ### Primary core task - stepper driver
// ### 
// #################################################################################

void updatePosition(bool direction) {
  float encoderStepsPerStepperStep = (ENCODER_PULS_PER_REVOLUTION * 4) / ((spindleMmPerRound() / (SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO / MICROSTEPS_PER_REVOLUTION)));
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

void startSpindel() {
  waitToSyncSpindel = true;
  isSpindelEnabled = true;
}

void stopSpindel() {
  isSpindelEnabled = false;
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
    for (int backlashStep = 1; backlashStep <= backlashInSteps; ++backlashStep) {
      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(100);
      digitalWrite(STEP_PIN, LOW);
      delayMicroseconds(400);
    }
  }

  if (directionChanged && !waitToSyncSpindel && !isJog) {
    waitToSyncSpindel = true;
    Serial.println("Start syncing spindle...");
  }

  if (waitToSyncSpindel) {
    if ((int)encoderDeg <= 5) {
      waitToSyncSpindel = false;
      Serial.println("Spindle in sync!");
      // Spindel in sync
    } else {
      Serial.println((int)encoderDeg);
      encoderLastSteps = encoder.getCount();
      return false;
    }
  }

  if (dir) {  // clockwise
    if (!isnan(stepperTarget) && (abs(stepperTarget - stepperPosition) <= THRESHOLD) && !directionChanged && !isJog) {
      Serial.println("Target reached");
      stopSpindel();
      return false; // Target reached!
    }
    stepperPosition += SINGLE_STEP;
    updatePosition(true);
  } else { // counterclockwise
    if (!isnan(stepperTarget) && (abs(stepperTarget - stepperPosition) <= THRESHOLD) && !directionChanged && !isJog) {
      Serial.println("Target reached");
      stopSpindel();
      return false; // Target reached!
    }
    stepperPosition -= SINGLE_STEP;
    updatePosition(false);
  }

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

  // max RPM calculation
  maxRpm = abs((MAX_MOTOR_SPEED * 60) / ((spindleMmPerRound() / (SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO)) * 200));

  // Stepperschritte pro Drehzahlencoder Schritt berechnen
  stepperStepsPerEncoderSteps = ((spindleMmPerRound() / (SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO / MICROSTEPS_PER_REVOLUTION))) / (ENCODER_PULS_PER_REVOLUTION * 4);

  // calculate rpm
  if (millis() >= rpmMillisTemp + rpmMillisMeasure) {
    rpm = (abs(encoderAct - encoderLastUpm) * 60000 / (millis() - rpmMillisTemp)) / (ENCODER_PULS_PER_REVOLUTION * 4);
    encoderLastUpm = encoderAct;
    rpmMillisTemp = rpmMillisTemp + rpmMillisMeasure;

    // Stop the spindel if we are running to fast.
    if (rpm > maxRpm) {
      stopSpindel();
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

  if (autoMoveToZero) {
    // Auto move to zero handler 
    if (abs(stepperPosition) >= THRESHOLD) {
      bool direction = stepperPosition > 0 ? false : true;
      performBlockingStep(direction, autoMoveToZeroMultiplier);
      autoMoveToZeroMultiplier = min(autoMoveToZeroMultiplier + autoMoveIncreaseStep, maxSpeedMultiplier);
    } else {
      autoMoveToZeroMultiplier = 1.0f;
      autoMoveToZero = false;
    }
  } else if (currentJogMode != neutral) {
    // Jog handler 
    bool direction = currentJogMode == left ? true : false;
    performBlockingStep(direction, jogCurrentSpeedMultiplier);
    jogCurrentSpeedMultiplier = min(jogCurrentSpeedMultiplier + jogIncreaseStep, maxSpeedMultiplier);
  } else {
    // Spindle step handler
    if (abs(stepsToDo) >= 1 && !stepPinIsOn) {
      bool direction = stepsToDo > 0;

      if (isSpindelEnabled) {
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