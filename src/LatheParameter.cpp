#include "LatheParameter.h"
#include <Preferences.h>

#define KEY_BACKLASH "backlash"
#define KEY_FEED_INDEX "feedIndex"
#define KEY_METRIC_FEED "metricFeed"
#define KEY_INVERT_FEED "invertFeed"

Preferences preferences;

Pitch availableMetricFeeds[] = {
  Pitch::fromMetric(0.02f),
  Pitch::fromMetric(0.05f),
  Pitch::fromMetric(0.1f),
  Pitch::fromMetric(0.15f),
  Pitch::fromMetric(0.2f),
  Pitch::fromMetric(0.25f),
  Pitch::fromMetric(0.3f),
  Pitch::fromMetric(0.5f),
  Pitch::fromMetric(0.75f),
  Pitch::fromMetric(0.8f),
  Pitch::fromMetric(1.0f),
  Pitch::fromMetric(1.25f),
  Pitch::fromMetric(1.5f),
  Pitch::fromMetric(2.0f),
  Pitch::fromMetric(2.5f)
};
Pitch availableImperialFeeds[] = {
  Pitch::fromImperial(48),
  Pitch::fromImperial(40),
  Pitch::fromImperial(32),
  Pitch::fromImperial(28),
  Pitch::fromImperial(24),
  Pitch::fromImperial(20),
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

LatheParameter::LatheParameter() {
    preferences.begin("settings", false);
    this->backlashValue = preferences.getInt(KEY_BACKLASH, 20);
    this->feedIndexValue = preferences.getInt(KEY_FEED_INDEX, 0);
    this->metricFeedValue = preferences.getBool(KEY_METRIC_FEED, true);
    this->invertFeedValue = preferences.getBool(KEY_INVERT_FEED, false);

    this->rpmValue = 0;
    this->maxRpmValue = 0;
}

int LatheParameter::backlash() {
    return this->backlashValue;
}
void LatheParameter::setBacklash(int value) {
    this->backlashValue = value;
    preferences.putInt(KEY_BACKLASH, value);
}

unsigned int LatheParameter::feedIndex() {
    return this->feedIndexValue;
}
void LatheParameter::setFeedIndex(unsigned int value) {
    this->feedIndexValue = value;
    preferences.putUInt(KEY_FEED_INDEX, value);
}

bool LatheParameter::isMetricFeed() {
    return this->metricFeedValue;
}
void LatheParameter::setMetricFeed(bool value) {
    this->metricFeedValue = value;
    preferences.putBool(KEY_METRIC_FEED, value);
}

bool LatheParameter::isInvertFeed() {
    return this->invertFeedValue;
}
void LatheParameter::setInvertFeed(bool value) {
    this->invertFeedValue = value;
    preferences.putBool(KEY_INVERT_FEED, value);
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


Pitch LatheParameter::spindlePitch() {
  Pitch pitch;
  int feedIndex = this->feedIndex();
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