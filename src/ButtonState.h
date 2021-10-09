#include <Arduino.h>

enum ButtonState { readyToTrigger, pressedDown, recognizedShort, recognizedLong, handled, listenOnlyForLong };
struct ButtonConfig { 
  int pin; 
  ButtonState state;
  int64_t pressTime;
  int64_t releaseTime;

  void handled() {
    this->state = ButtonState::handled;
  }

  void handledAndAcceptMoreLong() {
    this->state = ButtonState::listenOnlyForLong;
  }
};