#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Encoder.h>
#include <Preferences.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     0 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32 0x7A

#define ENCODER_A 14 // Drehzahlencoder Pin A
#define ENCODER_B 27 // Drehzahlencoder Pin B
#define ENCODER_PULS_PER_REVOLUTION 600 // Drehzahlencoder Pin B

#define DIR_PIN 4
#define STEP_PIN 2
#define STEPPER_GEAR_RATIO 1.0f/2.0f
#define MICROSTEPS_PER_REVOLUTION 400
#define SPINDEL_THREAD_PITCH 1.5f // value in mm

// Buttons
#define BUTTON_ADD_PIN 23
#define BUTTON_ADD_INDEX 0

#define BUTTON_REMOVE_PIN 18
#define BUTTON_REMOVE_INDEX 1

#define BUTTON_A_PIN 13
#define BUTTON_A_INDEX 2

#define BUTTON_B_PIN 17
#define BUTTON_B_INDEX 3

#define BUTTON_JOG_LEFT_PIN 33
#define BUTTON_JOG_RIGHT_PIN 35

enum ButtonState { idl, pressed, releasedShort, releasedLong, handledShort, handledLong };
struct ButtonConfig { 
  int pin; 
  ButtonState state;
  int64_t pressTime;
  int64_t releaseTime;
};

enum JOGMode { left, neutral, right };
JOGMode currentJogMode = neutral;
int64_t disableJogUntil = 0;
bool hasJogPausedAtTarget = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TaskHandle_t userInterfaceTask;
Preferences preferences;

// RPM Encoder
ESP32Encoder encoder;
int64_t encoderLastUpm = 0;     // letzte Position des Encoders (UPM) in Steps *4 (Quad)
int64_t encoderAct = 0;      // aktue Position des Encoders in Steps *4 (Quad)
int64_t encoderLastSteps = 0; // Tempvariable für encoderDoSteps

// RPM
int     rpm = 0; // Umdrehung pro Minute
int     rpmMillisMeasure = 500; // Messzeit für UPM in Millisekunden
int     rpmMillisTemp = 0; // Tempspeicher für millis
int64_t maxRpm = 0; // maximale, fehlerfreie UPM für Drehbank. Wird errechnet aus dem Vorschub

// Lathe parameters
int motorMaxSpeed = 2500; // Höchstgeschwindigkeit
float spindleMmPerRound = 1.5f; // Leitspindelweg pro umdrehung in mm aka feed
float stepperStepsPerEncoderSteps = 0.0f; // Leitspindel Schrittmotor Schritte pro Drehschritt (errechnet sich aus stepper Steps, threadPitch und spindleMmPerRound)
bool directionChanged = false;
float stepperPosition = 0.0f; // in mm
float stepperTarget = NAN; // in mm
bool waitToSyncSpindel = false;
int64_t lastStepTime = 0;
int64_t stepPinIsOn = false;
int64_t encoderCalcBase = 0;
int backlashInSteps = 20;
bool stepDelayDirection = true; // To reset stepDelayUs when direction changes.

// Degree calculation
int64_t encoderAbs=0;
bool zeroDeg=false; // wird auf true gesetzt wenn 0° Punkt erreicht
bool dispZeroDeg=false;
bool encoderDirection = false;
float encoderDeg = 0; // Winkel des encoders;

void writeToDisplay(String text) {
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(text);
  display.display();
}

void IRAM_ATTR encoderISR() { 
  encoderAbs = abs(encoder.getCount()/4)%(ENCODER_PULS_PER_REVOLUTION);
  if(encoderAbs==0){
    zeroDeg=true;
    dispZeroDeg=true;
  }
}

void endSingleStep() {
  digitalWrite(STEP_PIN, LOW);
}

// Moves the stepper.
void startSingleStep(bool dir, bool isJog) {
  float threshold = 0.001;
  bool needBacklashCompensation = false;
  // Start slow if direction changed.
  if (stepDelayDirection != dir) {
    // stepDelayUs = PULSE_MAX_US;
    stepDelayDirection = dir;
    digitalWrite(DIR_PIN, dir ? HIGH : LOW);
    Serial.println("Direction change");
    directionChanged = true;
    needBacklashCompensation = true;
  }

  //Compensate Backlash
  if (needBacklashCompensation) {
    for (int backlashStep = 1; backlashStep <= backlashInSteps; ++backlashStep) {
      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(500);
      digitalWrite(STEP_PIN, LOW);
      delayMicroseconds(500);
    }
  }

  if (directionChanged && !isnan(stepperTarget)) {
    waitToSyncSpindel = true;
  }

  if (waitToSyncSpindel && !isJog) {
    if (encoderDeg <= 1) {
      waitToSyncSpindel = false;
      // Spindel in sync
    } else {
      Serial.println(encoderDeg);
      return;
    }
  } else if (isJog && !isnan(stepperTarget) && !hasJogPausedAtTarget) {
    if (abs(stepperTarget - stepperPosition) <= threshold) {
      hasJogPausedAtTarget = true;
      disableJogUntil = millis() + 500;
      return;
    }
  }

  if (disableJogUntil > millis()) {
    return; // Jog has to pause
  } else {
    hasJogPausedAtTarget = false;
  }

    if (dir) {  // clockwise
      if (!isnan(stepperTarget) && (abs(stepperTarget - stepperPosition) <= threshold) && !directionChanged && !isJog) {
        stepperPosition = stepperTarget;
        return; // Target reached!
      }
      stepperPosition += (SPINDEL_THREAD_PITCH / MICROSTEPS_PER_REVOLUTION);
    } else { // counterclockwise
      if (!isnan(stepperTarget) && (abs(stepperTarget - stepperPosition) <= threshold) && !directionChanged && !isJog) {
        stepperPosition = stepperTarget;
        return; // Target reached!
      }
      stepperPosition -= (SPINDEL_THREAD_PITCH / MICROSTEPS_PER_REVOLUTION);
    }

    digitalWrite(STEP_PIN, HIGH);
    // Don't wait during the last step, it will pass by itself before we get back to stepping again.
    // This condition is the reason moving left-right is limited to 600rpm but with ELS On and spindle
    // gradually speeding up, stepper can go to ~1200rpm.
    // if (i < steps - 1) {
    //   Serial.println(stepDelayUs);
    //   delayMicroseconds(stepDelayUs);
    // }
  // }
  directionChanged = false;
}

ButtonConfig buttonConfigs[4] = {
  ButtonConfig{BUTTON_ADD_PIN, idl, 0, 0},
  ButtonConfig{BUTTON_REMOVE_PIN, idl, 0, 0},
  ButtonConfig{BUTTON_A_PIN, idl, 0, 0},
  ButtonConfig{BUTTON_B_PIN, idl, 0, 0}
};

void resetButtonToIdl(ButtonConfig* button) {
  if (button->state == releasedLong) {
    button->state = handledLong;
    button->pressTime = button->releaseTime;
    button->releaseTime = 0;
  } else if (button->state == releasedShort) {
    button->state = handledShort;
    button->pressTime = 0;
    button->releaseTime = 0;
  } else {
    button->state = idl;
    button->pressTime = 0;
    button->releaseTime = 0;
  }
}

bool isLongPress(int64_t start, int64_t end) {
  if (end - start >= 500) {
    return true;
  }
  return false;
}

// TODO multiple long press detection not working?
void checkButtonState(ButtonConfig* button) {
  if ((button->state == handledShort || button->state == handledLong) && digitalRead(button->pin) == HIGH) {
    button->state = idl;
    resetButtonToIdl(button);
  } else if (button->state == idl) {
    if (digitalRead(button->pin) == HIGH) { // Button has not pushed yet and is still idl
      return;
    } else {
      button->state = pressed;
      button->pressTime = millis();
    }
  } else if (button->state == pressed) {
    if (digitalRead(button->pin) == HIGH) { // Button released
      button->releaseTime = millis();
      if (isLongPress(button->pressTime, button->releaseTime)) {
        button->state = releasedLong;
      } else {
        button->state = releasedShort;
      }
    } else {
      Serial.println("OK");
      if (isLongPress(button->pressTime, millis())) {
        button->state = releasedLong;
      }
      // Currently it is a short press but we do not know yet
    }
  }
}

void secondCoreTask( void * parameter) {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  preferences.begin("settings", false);
  backlashInSteps = preferences.getInt("backlash", backlashInSteps);
  spindleMmPerRound = preferences.getFloat("feed", spindleMmPerRound);

  pinMode(BUTTON_ADD_PIN, INPUT_PULLUP);
  pinMode(BUTTON_REMOVE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
  pinMode(BUTTON_JOG_LEFT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_JOG_RIGHT_PIN, INPUT_PULLUP);

  //TODO: Setup buttons

  while (true) {

    checkButtonState(&buttonConfigs[BUTTON_ADD_INDEX]);
    checkButtonState(&buttonConfigs[BUTTON_REMOVE_INDEX]);
    checkButtonState(&buttonConfigs[BUTTON_A_INDEX]);
    checkButtonState(&buttonConfigs[BUTTON_B_INDEX]);

    if (buttonConfigs[BUTTON_ADD_INDEX].state == releasedShort) {
      spindleMmPerRound = min(spindleMmPerRound + 0.05, 5.0);
      preferences.putFloat("feed", spindleMmPerRound);
      resetButtonToIdl(&buttonConfigs[BUTTON_ADD_INDEX]);
    } else if (buttonConfigs[BUTTON_ADD_INDEX].state == releasedLong) {
      backlashInSteps += 1;
      preferences.putInt("backlash", backlashInSteps);
      resetButtonToIdl(&buttonConfigs[BUTTON_ADD_INDEX]);
    }

    if (buttonConfigs[BUTTON_REMOVE_INDEX].state == releasedShort) {
      spindleMmPerRound = max(spindleMmPerRound - 0.05, 0.05);
      preferences.putFloat("feed", spindleMmPerRound);
      resetButtonToIdl(&buttonConfigs[BUTTON_REMOVE_INDEX]);
    } else if (buttonConfigs[BUTTON_REMOVE_INDEX].state == releasedLong) {
      backlashInSteps = max(0, backlashInSteps - 1);
      preferences.putInt("backlash", backlashInSteps);
      resetButtonToIdl(&buttonConfigs[BUTTON_REMOVE_INDEX]);
    }

    if (buttonConfigs[BUTTON_A_INDEX].state == releasedShort) {
      if (isnan(stepperTarget)) {
        stepperTarget = stepperPosition;
        waitToSyncSpindel = true;
      } else {
        stepperTarget = NAN;
        waitToSyncSpindel = false;
      }
      resetButtonToIdl(&buttonConfigs[BUTTON_A_INDEX]);
    } else if (buttonConfigs[BUTTON_A_INDEX].state == releasedLong) {
      Serial.println("Long Press A");
      resetButtonToIdl(&buttonConfigs[BUTTON_A_INDEX]);
    }

    if (buttonConfigs[BUTTON_B_INDEX].state == releasedShort) {
      stepperTarget += stepperPosition;
      stepperPosition = 0.0f;
      resetButtonToIdl(&buttonConfigs[BUTTON_B_INDEX]);
    } else if (buttonConfigs[BUTTON_B_INDEX].state == releasedLong) {
      Serial.println("Long Press B");
      resetButtonToIdl(&buttonConfigs[BUTTON_B_INDEX]);
    }

    if (digitalRead(BUTTON_JOG_LEFT_PIN) == LOW) {
      currentJogMode = left;
    } else if (digitalRead(BUTTON_JOG_RIGHT_PIN) == LOW) {
      currentJogMode = right;
    } else {
       currentJogMode = neutral;
    }
  
    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
    display.println("RPM " + String(rpm));
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.println("Feed " + String(spindleMmPerRound));
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

void setup() {
  Serial.begin(115200);

  ESP32Encoder::useInternalWeakPullResistors = UP;
  encoder.attachFullQuad(ENCODER_A, ENCODER_B);
  encoder.clearCount();

  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);

  xTaskCreatePinnedToCore(
    secondCoreTask, /* Function to implement the task */
    "UserInterface", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    2,  /* Priority of the task */
    &userInterfaceTask,  /* Task handle. */
    0); /* Core where the task should run */
}

int64_t getEncoderCount() {
  if (encoderDirection == false) {
    return encoderCalcBase + encoder.getCount();
  } else {
    return encoderCalcBase - encoder.getCount();
  }
}
void setEncoderDirection(bool dir) {
  encoderCalcBase = encoder.getCount();
  encoder.setCount(0);
  encoderLastSteps = encoderCalcBase;
  encoderDirection = dir;
}

void loop() {

  // Serial.println(maxUpm);

  // Aktuelle Drehzahlencoder Position
  encoderAct = getEncoderCount();

  // max RPM calculation
  maxRpm = (motorMaxSpeed * 60) / ((spindleMmPerRound / (SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO)) * 200);

  // Stepperschritte pro Drehzahlencoder Schritt berechnen
  stepperStepsPerEncoderSteps = (spindleMmPerRound / ((SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO) / MICROSTEPS_PER_REVOLUTION)) / (ENCODER_PULS_PER_REVOLUTION * 4);

  //Motor steuern
  int encoderActDifference = encoderAct - encoderLastSteps;
  int stepsToDo = encoderActDifference * stepperStepsPerEncoderSteps;
  if (rpm == 0 && currentJogMode != neutral) {
    if (currentJogMode == left) {
      startSingleStep(true, true);
    } else {
      startSingleStep(false, true);
    }
    delayMicroseconds(500*2);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(500*2);
  } else if (abs(stepsToDo) >= 1) {

    if (abs(stepsToDo) >= 2) {
      Serial.println("!!!!! more than one Step to do !!!!!");
    }

    encoderLastSteps = encoderAct;
    bool direction = stepsToDo > 0;
    startSingleStep(direction, false);
    stepPinIsOn = true;
    lastStepTime = micros();
  } else if (abs((float)encoderActDifference * stepperStepsPerEncoderSteps) >= 0.5 && stepPinIsOn) {
      endSingleStep();
      stepPinIsOn = false;
  } else if (lastStepTime >= 20 && stepPinIsOn) {
      endSingleStep();
      stepPinIsOn = false;
  }

// Drehzahl berechnen
  if (millis() >= rpmMillisTemp + rpmMillisMeasure) {
    rpm = (abs(encoderAct - encoderLastUpm) * 60000 / (millis() - rpmMillisTemp)) / (ENCODER_PULS_PER_REVOLUTION * 4);
    encoderLastUpm = encoderAct;
    rpmMillisTemp = rpmMillisTemp + rpmMillisMeasure;
  }

    // Winkel berechnen
  encoderDeg = float(encoderAct % (ENCODER_PULS_PER_REVOLUTION * 4)) * 360 / (ENCODER_PULS_PER_REVOLUTION * 4);
  if (encoderDeg < 0) {
    encoderDeg = 360 + encoderDeg;
  }
}