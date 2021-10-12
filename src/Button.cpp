#include "Button.h"

bool Button::isLongPress(int64_t start, int64_t end) {
  if (end - start >= 500) {
    return true;
  }
  return false;
}

bool Button::isShortPress(int64_t start, int64_t end) {
  if (end - start >= 50) {
    return true;
  }
  return false;
}

void Button::markButtonAsLongPress(ButtonConfig* button) {
  button->state = recognizedLong;
  button->pressTime = millis();
  button->releaseTime = 0;
}

ButtonState Button::checkButtonState(ButtonConfig* button) {
  if (button->state == listenOnlyForLong) {
    if (digitalRead(button->pin) == HIGH) {
      button->state = handled;
    } else {
      if (isLongPress(button->pressTime, millis())) {
        markButtonAsLongPress(button);
      }
    }
  } else if (button->state == handled && digitalRead(button->pin) == HIGH) {
    button->state = readyToTrigger;
  } else if (button->state == readyToTrigger) {
    if (digitalRead(button->pin) == LOW) {
      button->state = pressedDown;
      button->pressTime = millis();
    }
  } else if (button->state == pressedDown) {
    if (digitalRead(button->pin) == HIGH) { // Button released
      button->releaseTime = millis();
      if (isLongPress(button->pressTime, button->releaseTime)) {
        markButtonAsLongPress(button);
      } else if (isShortPress(button->pressTime, button->releaseTime)) {
        button->state = recognizedShort;
      } else {
        //Some wired noise. Do nothing
      }
    } else {
      if (isLongPress(button->pressTime, millis())) {
        markButtonAsLongPress(button);
      }
      // else: Currently, it is not possible to distinguish between long and short.
    }
  }
  return button->state;
}