// Granular.h
#pragma once
#include <vector>
#include <array>
#include <random>
#include <cmath>
#include <algorithm>

namespace Steinberg {
namespace oscilleon {

static constexpr int kMaxGrainsEngine = 32;

struct Grain {
    bool   active   = false;
    double readPos  = 0.0;
    double rate     = 1.0;
    int    age      = 0;
    int    duration = 0;
    float  panL     = 1.0f;
    float  panR     = 1.0f;
    int    bufN     = 0;
};

struct GranularParams {
    bool   freeze             = false;
    float  inputGain          = 1.0f;
    float  feedback           = 0.0f;
    int    effectiveBuf       = 0;
    int    maxGrains          = 16;
    float  position           = 0.0f;
    float  increment          = 0.0f;
    double grainInterval      = 5512.0;
    int    grainDuration      = 4410;
    double pitchRate          = 1.0;
    float  reverseProbability = 0.0f;
    float  pan                = 0.0f;
    float  volume             = 1.0f;
    float  granulatorMix      = 0.8f;
};

class Granular {
public:
    void prepare(double sampleRate, int maxBufSamples);
    void reset();
    void setParams(const GranularParams& p);
    void setEffectiveBuf(int newN);
    void processSample(float inL, float inR, float& wetL, float& wetR);

    int                       getWritePos() const { return m_writePos; }
    const std::vector<float>& getBufL()     const { return m_bufL; }
    const std::vector<float>& getBufR()     const { return m_bufR; }

private:
    float hannWindow(int age, int duration) const;
    float readBuf(const std::vector<float>& buf, double pos, int N) const;
    void  scheduleGrain();
    void  clearGrains();

    static constexpr double kGuardMargin = 256.0;

    double m_sampleRate  = 44100.0;
    int    m_writePos    = 0;
    double m_grainTimer  = 0.0;
    double m_scanPos     = 0.0;

    std::vector<float>                   m_bufL, m_bufR;
    std::array<Grain, kMaxGrainsEngine>  m_grains{};
    std::mt19937                         m_rng{ std::random_device{}() };

    GranularParams m_p;

    float m_targetGrainScale = 1.0f;
    float m_smoothedScale    = 1.0f;
    float m_smoothedVolume   = 1.0f;
    float m_smoothedMix      = 0.8f;
    float m_smoothCoeff      = 0.0f;
};

} // namespace oscilleon
} // namespace Steinberg