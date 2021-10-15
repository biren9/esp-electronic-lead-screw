#ifndef __Pitch_h_
#define __Pitch_h_

#include "Arduino.h"

struct Pitch {
    String name;
    float metricFeed;

    static Pitch fromMetric(float pitch) {
        return Pitch{String(pitch)+" mm", pitch};
    }

    static Pitch fromImperial(uint8_t tpi) {
        return Pitch{String(tpi)+" TPI", 25.4f/(float)tpi};
    }
};

#endif