#ifndef __LatheParameter_h_
#define __LatheParameter_h_
#include "Pitch.h"

class LatheParameter {

    public:
        LatheParameter();

        // Persisted
        int backlash();
        void setBacklash(int value);

        unsigned int feedIndex();
        void setFeedIndex(unsigned int value);

        bool isMetricFeed();
        void setMetricFeed(bool value);

        bool isInvertFeed();
        void setInvertFeed(bool value);

        // Not persisted
        int rpm();
        int maxRpm();
        void setRpm(int value);
        void setMaxRpm(int value);

        // Calculated parameters
        Pitch spindlePitch();
        unsigned int availablePitches();

    private:
        int backlashValue;
        unsigned int feedIndexValue;
        bool metricFeedValue;
        bool invertFeedValue;

        int rpmValue;
        int maxRpmValue;
};

#endif