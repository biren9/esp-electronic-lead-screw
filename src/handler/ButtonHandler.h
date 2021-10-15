#ifndef __ButtonHandler_h_
#define __ButtonHandler_h_

#include "LatheParameter.h"

class ButtonHandler {
    public:
        ButtonHandler();
        void handleButtons(LatheParameter* latheParameter);

    private:
        void handleButtonAdd(LatheParameter* latheParameter, int buttonIndex);
        void handleButtonRemove(LatheParameter* latheParameter, int buttonIndex);
        void handleButtonTarget(LatheParameter* latheParameter, int buttonIndex);
        void handleButtonPosition(LatheParameter* latheParameter, int buttonIndex);
        void handleButtonJog(LatheParameter* latheParameter, int buttonLeftPin, int buttonRightPin);
};

#endif