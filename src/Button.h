#ifndef __Button_h_
#define __Button_h_

#include <Arduino.h>

enum ButtonState { readyToTrigger, pressedDown, recognizedShort, recognizedLong, handled, listenOnlyForLong };
struct ButtonConfig { 
  int pin; 
  ButtonState state;
  unsigned long pressTime;
  unsigned long releaseTime;

  void handled() {
    this->state = ButtonState::handled;
  }

  void handledAndAcceptMoreLong() {
    this->state = ButtonState::listenOnlyForLong;
  }
};

class Button {
    public:
        static ButtonState checkButtonState(ButtonConfig*);

    private:
        static bool isLongPress(unsigned long start, unsigned long end);
        static bool isShortPress(unsigned long start, unsigned long end);
        static void markButtonAsLongPress(ButtonConfig* button);
};
#endif