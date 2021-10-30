#ifndef __ButtonHandler_h_
#define __ButtonHandler_h_

#include "LatheParameter.h"
#include "Button.h"
#include "Button.h"

class ButtonHandler {
    public:
        ButtonHandler(uint8_t pinAdd, uint8_t pinRemove, uint8_t pinTarget, uint8_t pinPosition, uint8_t pinJogLeft, uint8_t pinJogRight);
        void handleButtons(LatheParameter* latheParameter);

    private:
        void handleButtonAdd(LatheParameter* latheParameter, ButtonConfig* button);
        void handleButtonRemove(LatheParameter* latheParameter, ButtonConfig* button);
        void handleButtonTarget(LatheParameter* latheParameter, ButtonConfig* button);
        void handleButtonPosition(LatheParameter* latheParameter, ButtonConfig* button);
        void handleButtonJog(LatheParameter* latheParameter, ButtonConfig* buttonLeft, ButtonConfig* buttonRight);
        ButtonConfig buttonConfigs[6];
};

#endif