#ifndef __LatheParameter_h_
#define __LatheParameter_h_
#include "Pitch.h"
#include "Setting.h"
#include "JogMode.h"
#include "HandEncoderHighlight.h"
#include <Preferences.h>

class LatheParameter {

    public:
        explicit LatheParameter(Preferences* preferences);

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

        void setRelativHandEncoderSteps(int value);
        int relativHandEncoderSteps();
        void resetRelativHandEncoderSteps();

        // Calculated parameters
        Pitch spindlePitch();
        unsigned int availablePitches();

        void startSpindel();
        void stopSpindel(int8_t code);
        bool isSpindelEnabled();
        bool isSpindelInSync();
        void setSpindelInSync();

        SettingMode settingMode();
        void setSettingMode(SettingMode value);

        HandEncoderHighlight handEncoderHighlight();
        void setHandEncoderHighlight(HandEncoderHighlight value);

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

        int8_t currentStopCode();

    private:
        int8_t stopCode;
        Preferences* preferences;
        int backlashValue;
        unsigned int feedIndexValue;
        bool metricFeedValue;
        bool invertFeedValue;

        int rpmValue;
        int maxRpmValue;

        int relativHandEncoderStepsValue;

        bool isSpindelEnabledValue;
        bool isSpindelInSyncValue;

        HandEncoderHighlight handEncoderHighlightValue;
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