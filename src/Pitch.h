#include "Arduino.h"

struct Pitch {
    String name;
    float metricFeed;

    static Pitch fromMetric(float pitch) {
        return Pitch{String(pitch)+"mm", pitch};
    }

    static float fromImperial(uint8_t tpi) {
        return 25.4f/(float)tpi;
    }
};