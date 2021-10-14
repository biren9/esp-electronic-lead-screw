#include "Arduino.h"

struct Pitch {
    String name;
    float metricFeed;

    static Pitch fromMetric(float pitch) {
        return Pitch{String(pitch)+"mm", pitch};
    }

    static Pitch fromImperial(uint8_t tpi) {
        return Pitch{String(tpi)+"TPI", 25.4f/(float)tpi};
    }
};

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