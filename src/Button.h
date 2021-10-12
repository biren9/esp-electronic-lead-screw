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

class Button {
    public:
        static ButtonState checkButtonState(ButtonConfig*);

    private:
        static bool isLongPress(int64_t start, int64_t end);
        static bool isShortPress(int64_t start, int64_t end);
        static void markButtonAsLongPress(ButtonConfig* button);
};