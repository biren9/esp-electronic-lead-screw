#ifndef __LatheParameter_h_
#define __LatheParameter_h_
#include "Pitch.h"
#include "Setting.h"
#include "JogMode.h"

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

        void startSpindel();
        void stopSpindel();
        bool isSpindelEnabled();
        bool isSpindelInSync();
        void setSpindelInSync();

        SettingMode settingMode();
        void setSettingMode(SettingMode value);

        float stepperPosition();
        void setStepperPosition(float value);
        float stepperTarget();
        void setStepperTarget(float value);

        JOGMode currentJogMode();
        void setCurrentJogMode(JOGMode value);
        float jogCurrentSpeedMultiplier();
        void setJogCurrentSpeedMultiplier(float value);
        int8_t jogReadCounter();
        void setJogReadCounter(int8_t value);

        bool isAutoMoveToZero();
        void setAutoMoveToZero(bool value);
        float autoMoveToZeroMultiplier();
        void setAutoMoveToZeroMultiplier(float value);

    private:
        int backlashValue;
        unsigned int feedIndexValue;
        bool metricFeedValue;
        bool invertFeedValue;

        int rpmValue;
        int maxRpmValue;

        bool isSpindelEnabledValue;
        bool isSpindelInSyncValue;

        SettingMode settingModeValue;

        float stepperPositionValue;
        float stepperTargetValue;

        JOGMode currentJogModeValue;
        float jogCurrentSpeedMultiplierValue;
        int8_t jogReadCounterValue;

        bool isAutoMoveToZeroValue;
        float autoMoveToZeroMultiplierValue;
};

#endif