#include "ButtonHandler.h"

int8_t maxCounter = 5;

ButtonHandler::ButtonHandler(uint8_t pinAdd, uint8_t pinRemove, uint8_t pinTarget, uint8_t pinPosition, uint8_t pinJogLeft, uint8_t pinJogRight) {
    pinMode(pinAdd, INPUT_PULLUP);
    pinMode(pinRemove, INPUT_PULLUP);
    pinMode(pinTarget, INPUT_PULLUP);
    pinMode(pinPosition, INPUT_PULLUP);
    pinMode(pinJogLeft, INPUT_PULLUP);
    pinMode(pinJogRight, INPUT_PULLUP);

    this->buttonConfigs[0] = ButtonConfig{pinAdd, readyToTrigger, 0, 0};
    this->buttonConfigs[1] = ButtonConfig{pinRemove, readyToTrigger, 0, 0};
    this->buttonConfigs[2] = ButtonConfig{pinTarget, readyToTrigger, 0, 0};
    this->buttonConfigs[3] = ButtonConfig{pinPosition, readyToTrigger, 0, 0};
    this->buttonConfigs[4] = ButtonConfig{pinJogLeft, readyToTrigger, 0, 0};
    this->buttonConfigs[5] = ButtonConfig{pinJogRight, readyToTrigger, 0, 0};
}

void ButtonHandler::handleButtons(LatheParameter* latheParameter) {
    this->handleEncoderUpdate(latheParameter);
    this->handleButtonAdd(latheParameter, &buttonConfigs[0]);
    this->handleButtonRemove(latheParameter, &buttonConfigs[1]);
    this->handleButtonTarget(latheParameter, &buttonConfigs[2]);
    this->handleButtonPosition(latheParameter, &buttonConfigs[3]);
    this->handleButtonJog(latheParameter, &buttonConfigs[4], &buttonConfigs[5]);
}

void ButtonHandler::handleButtonAdd(LatheParameter* latheParameter, ButtonConfig* button) {
    switch (Button::checkButtonState(button)) {
    case recognizedLong:
      if (latheParameter->settingMode() == SettingModeNone) {
        latheParameter->stopSpindel(3);
        latheParameter->setSettingMode(SettingModeSetting);
      } else {
        latheParameter->setSettingMode(Setting::next(latheParameter->settingMode()));
      }
      button->handledAndAcceptMoreLong();
      break;
    case recognizedShort: {
      switch (latheParameter->settingMode()) {
        case SettingModeNone: {
          unsigned int size = latheParameter->availablePitches();
          latheParameter->setFeedIndex(min(latheParameter->feedIndex()+1u, size-1u));
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
      button->handled();
      break;
    }
    default:
      break;
    }
}

void ButtonHandler::handleButtonRemove(LatheParameter* latheParameter, ButtonConfig* button) {
    switch (Button::checkButtonState(button)) {
    case recognizedLong:
      latheParameter->setSettingMode(SettingModeNone);
      button->handled();
      break;
    case recognizedShort:
      switch (latheParameter->settingMode()) {
        case SettingModeNone: {
          unsigned int newIndex;
          if (latheParameter->feedIndex() == 0u) {
            newIndex = 0;
          } else {
            newIndex = latheParameter->feedIndex()-1;
          }
          latheParameter->setFeedIndex(newIndex);
          break;
        }
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
      button->handled();
      break;
    default:
      break;
    }
}

void ButtonHandler::handleButtonTarget(LatheParameter* latheParameter, ButtonConfig* button) {
switch (Button::checkButtonState(button)) {
    case recognizedLong:
    if (isnan(latheParameter->stepperTarget())) {
        latheParameter->setStepperTarget(latheParameter->stepperPosition());
      } else {
        latheParameter->setStepperTarget(NAN);
      }
      button->handled();
      break;
    case recognizedShort:
      if (latheParameter->isSpindelEnabled()) {
        latheParameter->stopSpindel(4);
      } else {
        latheParameter->startSpindel();
      }
      button->handled();
      break;
    default:
      break;
    }
}

void ButtonHandler::handleButtonPosition(LatheParameter* latheParameter, ButtonConfig* button) {
    switch (Button::checkButtonState(button)) {
    case recognizedLong:
      latheParameter->setStepperPosition(0.0f);
      button->handled();
      break;
    case recognizedShort:
      latheParameter->stopSpindel(5);
      latheParameter->setAutoMoveToZeroMultiplier(1.0f);
      latheParameter->setAutoMoveToZero(!latheParameter->isAutoMoveToZero());
      button->handled();
      break;
    default:
      break;
    }
}

void ButtonHandler::handleButtonJog(LatheParameter* latheParameter, ButtonConfig* buttonLeft, ButtonConfig* buttonRight) {
    if (digitalRead(buttonLeft->pin) == LOW) {
      latheParameter->setJogReadCounter(max(latheParameter->jogReadCounter()-1, -maxCounter));
      if (latheParameter->currentJogMode() != left && latheParameter->jogReadCounter() == -maxCounter) {
        latheParameter->setJogCurrentSpeedMultiplier(1.0f);
        latheParameter->setCurrentJogMode(left);
      }
    
      latheParameter->stopSpindel(6);
    } else if (digitalRead(buttonRight->pin) == LOW) {
      latheParameter->setJogReadCounter(min(latheParameter->jogReadCounter()+1, -maxCounter));
      if (latheParameter->currentJogMode() != right && latheParameter->jogReadCounter() <= maxCounter) {
        latheParameter->setJogCurrentSpeedMultiplier(1.0f);
        latheParameter->setCurrentJogMode(right);
      }
      latheParameter->stopSpindel(7);
    } else {
      latheParameter->setJogReadCounter(0);
      latheParameter->setJogCurrentSpeedMultiplier(1.0f);
      latheParameter->setCurrentJogMode(neutral);
    }
}

void ButtonHandler::handleEncoderUpdate(LatheParameter* latheParameter) {
  int offset = latheParameter->relativHandEncoderSteps();
  latheParameter->resetRelativHandEncoderSteps();

  if (offset == 0) {
    // Nothing to handle!
    return;
  }

   switch (latheParameter->handEncoderHighlight()) {
     case HandEncoderHighlightJog: {
        if (offset < 0) {
          //handle jog left
        } else {
          //handle jog right
        }
        break;
     }
      case HandEncoderHighlightPitch: {
        unsigned int size = latheParameter->availablePitches();
        unsigned int newIndex;

        if (offset < 0) {
          if (latheParameter->feedIndex() == 0u) {
            newIndex = 0;
          } else {
            newIndex = latheParameter->feedIndex()-1;
          }
        } else {
          newIndex = min(latheParameter->feedIndex()+(unsigned int)offset, size-1u);
        }
        latheParameter->setFeedIndex(newIndex);
        break;
      }
      case HandEncoderHighlightTarget: {
        latheParameter->setStepperTarget(latheParameter->stepperTarget() + offset/100.f);
        break;
      }
   }
}