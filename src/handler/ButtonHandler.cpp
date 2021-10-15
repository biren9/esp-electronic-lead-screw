#include "ButtonHandler.h"
#include "config.h"
#include "Button.h"

ButtonConfig buttonConfigs[4] = {
  ButtonConfig{BUTTON_ADD_PIN, readyToTrigger, 0, 0},
  ButtonConfig{BUTTON_REMOVE_PIN, readyToTrigger, 0, 0},
  ButtonConfig{BUTTON_TARGET_PIN, readyToTrigger, 0, 0},
  ButtonConfig{BUTTON_POSITION_PIN, readyToTrigger, 0, 0}
};

int8_t maxCounter = 5;

ButtonHandler::ButtonHandler() {
    pinMode(BUTTON_ADD_PIN, INPUT_PULLUP);
    pinMode(BUTTON_REMOVE_PIN, INPUT_PULLUP);
    pinMode(BUTTON_TARGET_PIN, INPUT_PULLUP);
    pinMode(BUTTON_POSITION_PIN, INPUT_PULLUP);
    pinMode(BUTTON_JOG_LEFT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_JOG_RIGHT_PIN, INPUT_PULLUP);
}

void ButtonHandler::handleButtons(LatheParameter* latheParameter) {
    this->handleButtonAdd(latheParameter, BUTTON_ADD_INDEX);
    this->handleButtonRemove(latheParameter, BUTTON_REMOVE_INDEX);
    this->handleButtonTarget(latheParameter, BUTTON_TARGET_INDEX);
    this->handleButtonPosition(latheParameter, BUTTON_POSITION_INDEX);
    this->handleButtonJog(latheParameter, BUTTON_JOG_LEFT_PIN, BUTTON_JOG_RIGHT_PIN);
}

void ButtonHandler::handleButtonAdd(LatheParameter* latheParameter, int buttonIndex) {
    switch (Button::checkButtonState(&buttonConfigs[buttonIndex])) {
    case recognizedLong:
      if (latheParameter->settingMode() == SettingModeNone) {
        latheParameter->stopSpindel();
        latheParameter->setSettingMode(SettingModeSetting);
      } else {
        latheParameter->setSettingMode(Setting::next(latheParameter->settingMode()));
      }
      buttonConfigs[buttonIndex].handledAndAcceptMoreLong();
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
      buttonConfigs[buttonIndex].handled();
      break;
    }
    default:
      break;
    }
}

void ButtonHandler::handleButtonRemove(LatheParameter* latheParameter, int buttonIndex) {
    switch (Button::checkButtonState(&buttonConfigs[buttonIndex])) {
    case recognizedLong:
      latheParameter->setSettingMode(SettingModeNone);
      buttonConfigs[buttonIndex].handled();
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
      buttonConfigs[buttonIndex].handled();
      break;
    default:
      break;
    }
}

void ButtonHandler::handleButtonTarget(LatheParameter* latheParameter, int buttonIndex) {
switch (Button::checkButtonState(&buttonConfigs[buttonIndex])) {
    case recognizedLong:
    if (isnan(latheParameter->stepperTarget())) {
        latheParameter->setStepperTarget(latheParameter->stepperPosition());
      } else {
        latheParameter->setStepperTarget(NAN);
      }
      buttonConfigs[buttonIndex].handled();
      break;
    case recognizedShort:
      if (latheParameter->isSpindelEnabled()) {
        latheParameter->stopSpindel();
      } else {
        latheParameter->startSpindel();
      }
      buttonConfigs[buttonIndex].handled();
      break;
    default:
      break;
    }
}

void ButtonHandler::handleButtonPosition(LatheParameter* latheParameter, int buttonIndex) {
    switch (Button::checkButtonState(&buttonConfigs[buttonIndex])) {
    case recognizedLong:
      latheParameter->setStepperPosition(0.0f);
      buttonConfigs[buttonIndex].handled();
      
      break;
    case recognizedShort:
      latheParameter->stopSpindel();
      latheParameter->setAutoMoveToZeroMultiplier(1.0f);
      latheParameter->setAutoMoveToZero(latheParameter->isAutoMoveToZero());
      buttonConfigs[buttonIndex].handled();
      break;
    default:
      break;
    }
}

void ButtonHandler::handleButtonJog(LatheParameter* latheParameter, int buttonLeftPin, int buttonRightPin) {
    if (digitalRead(buttonLeftPin) == LOW) {
      latheParameter->setJogReadCounter(max(latheParameter->jogReadCounter()-1, -maxCounter));
      if (latheParameter->currentJogMode() != left && latheParameter->jogReadCounter() == -maxCounter) {
        latheParameter->setJogCurrentSpeedMultiplier(1.0f);
        latheParameter->setCurrentJogMode(left);
      }
    
      latheParameter->stopSpindel();
    } else if (digitalRead(buttonRightPin) == LOW) {
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
}