#include "DisplayHandler.h"
#include <Adafruit_SSD1306.h>
#include "config.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

DisplayHandler::DisplayHandler() {
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
  }
}

void DisplayHandler::updateDisplay(LatheParameter* latheParameter) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    switch (latheParameter->settingMode()) {
      case SettingModeNone: {// Operation mode
        display.setTextSize(2);
        display.println("RPM " + String(latheParameter->rpm()));
        display.setTextSize(1);
        display.println("Max " + String(latheParameter->maxRpm()));

        String feedName = latheParameter->spindlePitch().name;
        display.println("Feed " + String(feedName + " " + this->spindelState(latheParameter)));
        //display.println("Degree " + String(encoderDeg));
        if (!isnan(latheParameter->stepperTarget())) {
          display.println("Target " + String(latheParameter->stepperTarget()));
        }
        display.println("Position " + String(latheParameter->stepperPosition()));

        if (latheParameter->currentStopCode() != 0) {
          display.println("Code " + String(latheParameter->currentStopCode()));
        }

        break;
      }
      case SettingModeSetting:
        display.setTextSize(2);
        display.println("Setting");
        display.setTextSize(1);
        display.println("Author: Gil Biren");
        display.println("Version: " + VERSION);
        display.println("Compile date: " + BUILD_DATE);
        break;
      case SettingModeBacklash:
        display.setTextSize(2);
        display.println("Setting");
        display.setTextSize(1);
        display.println("Backlash: " + String(latheParameter->backlash()));
        break;
      case SettingModeMeasurementSystem: {
        display.setTextSize(2);
        display.println("Setting");
        display.setTextSize(1);
        String system = latheParameter->isMetricFeed() ? String("Metric") : String("Imperial");
        display.println("Unit: " + system);
        break;
      }
      case SettingModeInvertFeed: {
        display.setTextSize(2);
        display.println("Setting");
        display.setTextSize(1);
        String isInverted = latheParameter->isInvertFeed() ? String("Yes") : String("No");
        display.println("Invert Feed: " + isInverted);
        break;
      }
    }
    display.display();
}

String DisplayHandler::spindelState(LatheParameter* latheParameter) {
    if (latheParameter->isSpindelEnabled()) {
      if (!latheParameter->isSpindelInSync()) {
        return "Syncing";
      } else {
        return "On";
      }
    } else {
      return "Off";
    }
}