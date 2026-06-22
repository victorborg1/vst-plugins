#pragma once

#include <cmath>
#include <algorithm>

namespace dsp::util {



inline constexpr double pi = 3.14159265358979323846;
inline constexpr double twopi = 6.28318530717958647692;




template<typename T>
inline T clamp(T x, T a, T b) {
    return std::min(std::max(x, a), b);
}


template<typename T>
inline T lerp(T a, T b, T t) {
    return a + (b - a) * t;
}


template<typename T>
inline T map(T x, T inMin, T inMax, T outMin, T outMax) {
    if (inMax == inMin)
        return outMin;

    T t = (x - inMin) / (inMax - inMin);
    return lerp(outMin, outMax, t);
}


inline float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}


inline float linearToDb(float x) {
    return 20.0f * std::log10(std::max(x, 1e-8f));
}


inline float quantize(float x, int bits) {
    bits = std::clamp(bits, 1, 24);

    float levels = (float)(1 << bits);

    x = clamp(x, -1.0f, 1.0f);

    float u = x * 0.5f + 0.5f;
    u = std::round(u * (levels - 1)) / (levels - 1);

    return u * 2.0f - 1.0f;
}

template<typename T>
class Downsampler
{
public:
    void setStep(int s) {
        step = std::max(1, s);
    }

    void reset(T v = T(0)) {
        held = v;
        counter = 0;
    }

    T process(T input) {
        if (++counter >= step) {
            counter = 0;
            held = input;
        }
        return held;
    }

private:
    int step = 1;
    int counter = 0;
    T held = 0;
};


template<typename T>
inline T noteToFreq(T note) {
    return T(440) * std::exp2((note - 69) / T(12));
}


template<typename T>
inline T freqToNote(T freq) {
    return T(69) + T(12) * std::log2(freq / T(440));
}




}