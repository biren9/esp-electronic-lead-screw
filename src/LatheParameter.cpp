#include "LatheParameter.h"
#include <Preferences.h>

#define KEY_BACKLASH "backlash"
#define KEY_FEED_INDEX "feedIndex"
#define KEY_METRIC_FEED "metricFeed"
#define KEY_INVERT_FEED "invertFeed"

Preferences preferences;

LatheParameter::LatheParameter() {
    preferences.begin("settings", false);
    this->backlashValue = preferences.getInt(KEY_BACKLASH, 20);
    this->feedIndexValue = preferences.getInt(KEY_FEED_INDEX, 0);
    this->metricFeedValue = preferences.getBool(KEY_METRIC_FEED, true);
    this->invertFeedValue = preferences.getBool(KEY_INVERT_FEED, false);
}

int LatheParameter::backlash() {
    return this->backlashValue;
}
void LatheParameter::setBacklash(int value) {
    this->backlashValue = value;
    preferences.putInt(KEY_BACKLASH, value);
}

int LatheParameter::feedIndex() {
    return this->feedIndexValue;
}
void LatheParameter::setFeedIndex(int value) {
    this->feedIndexValue = value;
    preferences.putInt(KEY_FEED_INDEX, value);
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
