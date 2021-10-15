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
    String spindelState;
    if (latheParameter->isSpindelEnabled()) {
      if (!latheParameter->isSpindelInSync()) {
        spindelState = "Syncing";
      } else {
        spindelState = "On";
      }
    } else {
      spindelState = "Off";
    }


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
        display.println("Feed " + String(feedName + " " + spindelState));
        //display.println("Degree " + String(encoderDeg));
        if (!isnan(latheParameter->stepperTarget())) {
          display.println("Target " + String(latheParameter->stepperTarget()));
        }
        display.println("Position " + String(latheParameter->stepperPosition()));
        break;
      }
      case SettingModeSetting:
        display.setTextSize(2);
        display.println("Setting");
        display.setTextSize(1);
        display.println("Select an option");
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