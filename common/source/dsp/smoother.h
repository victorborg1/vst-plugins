#pragma once

#include <cmath>
#include <algorithm>



namespace dsp::smoother {

class OnePole {
public:
    float value = 0.0f;
    float target = 0.0f;
    float coeff = 0.05f;

    void setCoeff(float c) { coeff = c; }

    void reset(float v) {
        value = target = v;
    }

    void setTarget(float t) {
        target = t;
    }

    float process() {
        value += coeff * (target - value);
        return value;
    }
};


inline float smooth(float current, float target, float coeff) {
    return current + coeff * (target - current);
}

}