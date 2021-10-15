#ifndef __ButtonHandler_h_
#define __ButtonHandler_h_

#include "LatheParameter.h"
#include "Button.h"

class ButtonHandler {
    public:
        ButtonHandler();
        void handleButtons(LatheParameter* latheParameter);

    private:
        void handleButtonAdd(LatheParameter* latheParameter, ButtonConfig* button);
        void handleButtonRemove(LatheParameter* latheParameter, ButtonConfig* button);
        void handleButtonTarget(LatheParameter* latheParameter, ButtonConfig* button);
        void handleButtonPosition(LatheParameter* latheParameter, ButtonConfig* button);
        void handleButtonJog(LatheParameter* latheParameter, ButtonConfig* buttonLeft, ButtonConfig* buttonRight);
};

#endif