#ifndef __DisplayHandler_h_
#define __DisplayHandler_h_

#include "LatheParameter.h"

class DisplayHandler {
    public:
        DisplayHandler();
        void updateDisplay(LatheParameter* latheParameter);

    private:
        String spindelState(LatheParameter* latheParameter);
};

#endif