#include "LatheParameter.h"

#define KEY_BACKLASH "backlash"
#define KEY_FEED_INDEX "feedIndex"
#define KEY_METRIC_FEED "metricFeed"
#define KEY_INVERT_FEED "invertFeed"

Pitch availableMetricFeeds[] = {
    Pitch::fromMetric(0.02f),
    Pitch::fromMetric(0.05f),
    Pitch::fromMetric(0.1f),
    Pitch::fromMetric(0.15f),
    Pitch::fromMetric(0.2f),
    Pitch::fromMetric(0.25f),
    Pitch::fromMetric(0.3f),
    Pitch::fromMetric(0.35f),
    Pitch::fromMetric(0.4f),
    Pitch::fromMetric(0.45f),
    Pitch::fromMetric(0.5f),
    Pitch::fromMetric(0.6f),
    Pitch::fromMetric(0.7f),
    Pitch::fromMetric(0.75f),
    Pitch::fromMetric(0.8f),
    Pitch::fromMetric(0.9f),
    Pitch::fromMetric(1.0f),
    Pitch::fromMetric(1.25f),
    Pitch::fromMetric(1.5f),
    Pitch::fromMetric(1.75f),
    Pitch::fromMetric(2.0f),
    Pitch::fromMetric(2.5f)
};
Pitch availableImperialFeeds[] = {
    Pitch::fromImperial(62),
    Pitch::fromImperial(60),
    Pitch::fromImperial(48),
    Pitch::fromImperial(40),
    Pitch::fromImperial(36),
    Pitch::fromImperial(32),
    Pitch::fromImperial(30),
    Pitch::fromImperial(28),
    Pitch::fromImperial(26),
    Pitch::fromImperial(25),
    Pitch::fromImperial(24),
    Pitch::fromImperial(22),
    Pitch::fromImperial(20),
    Pitch::fromImperial(19),
    Pitch::fromImperial(18),
    Pitch::fromImperial(16),
    Pitch::fromImperial(14),
    Pitch::fromImperial(13),
    Pitch::fromImperial(12),
    Pitch::fromImperial(11),
    Pitch::fromImperial(10),
    Pitch::fromImperial(9),
    Pitch::fromImperial(8)
};

LatheParameter::LatheParameter(Preferences* preferences) {
    this->preferences = preferences;
    this->preferences->begin("settings", false);
    this->backlashValue = preferences->getInt(KEY_BACKLASH, 20);
    this->feedIndexValue = preferences->getUInt(KEY_FEED_INDEX, 0u);
    this->metricFeedValue = preferences->getBool(KEY_METRIC_FEED, true);
    this->invertFeedValue = preferences->getBool(KEY_INVERT_FEED, false);

    this->rpmValue = 0;
    this->maxRpmValue = 0;

    this->isSpindelEnabledValue = false;
    this->isSpindelInSyncValue = false;

    this->settingModeValue = SettingModeNone;

    this->stepperPositionValue = 0.0f;
    this->stepperTargetValue = NAN;

    this->currentJogModeValue = neutral;
    this->jogCurrentSpeedMultiplierValue = 1.0f;
    this->jogReadCounterValue = 0;

    this->isAutoMoveToZeroValue = false;
    this->autoMoveToZeroMultiplierValue = 1.0f;
    this->stopCode = 0;
}

int LatheParameter::backlash() {
    return this->backlashValue;
}
void LatheParameter::setBacklash(int value) {
    this->backlashValue = value;
    preferences->putInt(KEY_BACKLASH, value);
}

unsigned int LatheParameter::feedIndex() {
    return this->feedIndexValue;
}
void LatheParameter::setFeedIndex(unsigned int value) {
    this->feedIndexValue = value;
    preferences->putUInt(KEY_FEED_INDEX, value);
}

bool LatheParameter::isMetricFeed() {
    return this->metricFeedValue;
}
void LatheParameter::setMetricFeed(bool value) {
    this->metricFeedValue = value;
    preferences->putBool(KEY_METRIC_FEED, value);
}

bool LatheParameter::isInvertFeed() {
    return this->invertFeedValue;
}
void LatheParameter::setInvertFeed(bool value) {
    this->invertFeedValue = value;
    preferences->putBool(KEY_INVERT_FEED, value);
}


int LatheParameter::rpm() {
    return this->rpmValue;
}
int LatheParameter::maxRpm() {
    return this->maxRpmValue;
}

void LatheParameter::setRpm(int value) {
    this->rpmValue = value;
}
void LatheParameter::setMaxRpm(int value) {
    this->maxRpmValue = value;
}


void LatheParameter::setRelativHandEncoderSteps(int value) {
    this->relativHandEncoderStepsValue = value;
}
int LatheParameter::relativHandEncoderSteps() {
    return this->relativHandEncoderStepsValue;
}
void LatheParameter::resetRelativHandEncoderSteps() {
    this->relativHandEncoderStepsValue = 0;
}

Pitch LatheParameter::spindlePitch() {
  Pitch pitch;
  unsigned int feedIndex = this->feedIndex();
  if (this->isMetricFeed()) {
    pitch = availableMetricFeeds[feedIndex];
  } else {
    pitch = availableImperialFeeds[feedIndex];
  }
  if (this->isInvertFeed()) {
    pitch.metricFeed  *= -1.0f;
  }

  return pitch;
}
unsigned int LatheParameter::availablePitches() {
    if (this->isMetricFeed()) {
        return sizeof(availableMetricFeeds) / sizeof(Pitch);
    } else {
        return sizeof(availableImperialFeeds) / sizeof(Pitch);
    }
}


void LatheParameter::startSpindel() {
  if (this->settingModeValue == SettingModeNone) {
    this->isSpindelEnabledValue = true;
    this->stopCode = 0;
  }
}

void LatheParameter::stopSpindel(int8_t code) {
  this->stopCode = code;
  this->isSpindelEnabledValue = false;
  this->isSpindelInSyncValue = false;
}

bool LatheParameter::isSpindelEnabled() {
    return this->isSpindelEnabledValue;
}
bool LatheParameter::isSpindelInSync() {
    return this->isSpindelInSyncValue;
}
void LatheParameter::setSpindelInSync() {
    this->isSpindelInSyncValue = true;
}


SettingMode LatheParameter::settingMode() {
    return this->settingModeValue;
}
void LatheParameter::setSettingMode(SettingMode value) {
    this->settingModeValue = value;
}

HandEncoderHighlight LatheParameter::handEncoderHighlight() {
 return this->handEncoderHighlightValue;
}
void LatheParameter::setHandEncoderHighlight(HandEncoderHighlight value) {
    this->handEncoderHighlightValue = value;
}

float LatheParameter::stepperPosition() {
    return this->stepperPositionValue;
}
void LatheParameter::setStepperPosition(float value) {
    this->stepperPositionValue = value;
}
float LatheParameter::stepperTarget() {
    return this->stepperTargetValue;
}
void LatheParameter::setStepperTarget(float value) {
    this->stepperTargetValue = value;
}

JOGMode LatheParameter::currentJogMode() {
    return this->currentJogModeValue;
}
void LatheParameter::setCurrentJogMode(JOGMode value) {
    this->currentJogModeValue = value;
}
float LatheParameter::jogCurrentSpeedMultiplier() {
    return this->jogCurrentSpeedMultiplierValue;
}
void LatheParameter::setJogCurrentSpeedMultiplier(float value) {
    this->jogCurrentSpeedMultiplierValue = value;
}
int8_t LatheParameter::jogReadCounter() {
    return this->jogReadCounterValue;
}
void LatheParameter::setJogReadCounter(int8_t value) {
    this->jogReadCounterValue = value;
}


bool LatheParameter::isAutoMoveToZero() {
    return this->isAutoMoveToZeroValue;
}
void LatheParameter::setAutoMoveToZero(bool value) {
    this->isAutoMoveToZeroValue = value;
}
float LatheParameter::autoMoveToZeroMultiplier() {
    return this->autoMoveToZeroMultiplierValue;
}
void LatheParameter::setAutoMoveToZeroMultiplier(float value) {
    this->autoMoveToZeroMultiplierValue = value;
}

int8_t LatheParameter::currentStopCode() {
    return this->stopCode;
}