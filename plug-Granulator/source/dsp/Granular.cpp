#include "Granular.h"

namespace Steinberg {
namespace oscilleon {

void Granular::prepare(double sampleRate, int maxBufSamples) {
    m_sampleRate = sampleRate;

    m_smoothCoeff = 1.0f - static_cast<float>(
        std::exp(-1.0 / (sampleRate * 0.005)));

    m_bufL.assign(maxBufSamples, 0.f);
    m_bufR.assign(maxBufSamples, 0.f);
    reset();
}

void Granular::reset() {
    std::fill(m_bufL.begin(), m_bufL.end(), 0.f);
    std::fill(m_bufR.begin(), m_bufR.end(), 0.f);
    m_writePos   = 0;
    m_scanPos    = 0.0;
    m_grainTimer = 0.0;
    clearGrains();

    m_smoothedScale  = m_targetGrainScale;
    m_smoothedVolume = m_p.volume;
    m_smoothedMix    = m_p.granulatorMix;
}

void Granular::clearGrains() {
    for (auto& g : m_grains) g.active = false;
}

void Granular::setEffectiveBuf(int newN) {
    if (newN == m_p.effectiveBuf) return;
    
    m_p.effectiveBuf = newN;
    
    clearGrains();
    
    m_scanPos = static_cast<double>(m_writePos);
    m_grainTimer = m_p.grainInterval * 0.5;
}

void Granular::setParams(const GranularParams& p) {
    if (p.grainInterval < m_p.grainInterval)
        m_grainTimer = std::min(m_grainTimer, p.grainInterval);

    double oldOverlap = static_cast<double>(m_p.grainDuration) / std::max(m_p.grainInterval, 1.0);
    double newOverlap = static_cast<double>(p.grainDuration) / std::max(p.grainInterval, 1.0);
    if (std::abs(newOverlap - oldOverlap) > 0.001 || p.maxGrains != m_p.maxGrains) {
        double effectiveOverlap = std::min(newOverlap, static_cast<double>(p.maxGrains));
        m_targetGrainScale = 2.0f / static_cast<float>(std::max(1.0, effectiveOverlap));
    }

    m_p = p;
}

float Granular::hannWindow(int age, int duration) const {
    if (duration <= 1) return 1.f;
    float t = static_cast<float>(age) / static_cast<float>(duration - 1);
    return 0.5f * (1.f - std::cos(6.28318530f * t));
}

float Granular::readBuf(const std::vector<float>& buf, double pos, int N) const {
    if (N <= 0) return 0.f;

    pos = std::fmod(pos, static_cast<double>(N));
    if (pos < 0.0) pos += N;

    int   i1 = static_cast<int>(pos);
    int   i0 = (i1 - 1 + N) % N;
    int   i2 = (i1 + 1) % N;
    int   i3 = (i1 + 2) % N;
    float t  = static_cast<float>(pos - std::floor(pos));

    float p0 = buf[i0], p1 = buf[i1], p2 = buf[i2], p3 = buf[i3];
    float c0 = p1;
    float c1 = 0.5f * (p2 - p0);
    float c2 = p0 - 2.5f * p1 + 2.f * p2 - 0.5f * p3;
    float c3 = 0.5f * (p3 - p0) + 1.5f * (p1 - p2);
    return ((c3 * t + c2) * t + c1) * t + c0;
}

void Granular::scheduleGrain() {
    int N = m_p.effectiveBuf;
    if (N <= 0) return;

    int    activeCount = 0;
    Grain* slot        = nullptr;
    for (auto& g : m_grains) {
        if (g.active) ++activeCount;
        else if (!slot) slot = &g;
    }
    if (activeCount >= m_p.maxGrains || !slot) return;

    slot->active   = true;
    slot->age      = 0;
    slot->duration = m_p.grainDuration;
    slot->bufN     = N;

    double delay = m_p.position * static_cast<double>(N - 1) + m_scanPos;
    slot->readPos = std::fmod(
        static_cast<double>(m_writePos) - delay,
        static_cast<double>(N));
    if (slot->readPos < 0.0) slot->readPos += N;

    double forwardReadDistance = static_cast<double>(m_p.grainDuration) * std::abs(m_p.pitchRate);
    double guardSamples = std::min(forwardReadDistance + kGuardMargin, static_cast<double>(N) * 0.9);
    double distToHead = std::fmod(
        static_cast<double>(m_writePos) - slot->readPos + N,
        static_cast<double>(N));
    if (distToHead < guardSamples) {
        double newReadPos = std::fmod(
            static_cast<double>(m_writePos) - guardSamples + N,
            static_cast<double>(N));
        if (newReadPos < 0.0) newReadPos += N;
        double newDist = std::fmod(
            static_cast<double>(m_writePos) - newReadPos + N,
            static_cast<double>(N));
        if (newDist >= guardSamples) {
            slot->readPos = newReadPos;
        } else {
            slot->active = false;
            return;
        }
    }

    bool reversed = std::uniform_real_distribution<float>(0.f, 1.f)(m_rng)
                    < m_p.reverseProbability;
    slot->rate    = reversed ? -m_p.pitchRate : m_p.pitchRate;

    static constexpr float kPanSpread = 0.4f;
    float finalPan = std::clamp(
        m_p.pan + std::uniform_real_distribution<float>(-kPanSpread, kPanSpread)(m_rng),
        -1.f, 1.f);
    slot->panL = std::sqrt(std::max(0.f, 0.5f - finalPan * 0.5f));
    slot->panR = std::sqrt(std::max(0.f, 0.5f + finalPan * 0.5f));

    m_scanPos += m_p.increment * m_p.grainInterval;
    m_scanPos  = std::fmod(m_scanPos, static_cast<double>(N));
    if (m_scanPos < 0.0) m_scanPos += N;
}

void Granular::processSample(float inL, float inR, float& wetL, float& wetR) {
    int N = m_p.effectiveBuf;

    if (!m_p.freeze && N > 0) {
        m_bufL[m_writePos] = m_p.inputGain * inL + m_p.feedback * m_bufL[m_writePos];
        m_bufR[m_writePos] = m_p.inputGain * inR + m_p.feedback * m_bufR[m_writePos];
    }

    m_smoothedScale  += m_smoothCoeff * (m_targetGrainScale  - m_smoothedScale);
    m_smoothedVolume += m_smoothCoeff * (m_p.volume          - m_smoothedVolume);
    m_smoothedMix    += m_smoothCoeff * (m_p.granulatorMix   - m_smoothedMix);

    m_grainTimer -= 1.0;
    while (m_grainTimer <= 0.0) {
        scheduleGrain();
        m_grainTimer += m_p.grainInterval;
    }

    wetL = wetR = 0.f;
    for (auto& g : m_grains) {
        if (!g.active) continue;

        float env = hannWindow(g.age, g.duration) * m_smoothedVolume;
        float sampleL = readBuf(m_bufL, g.readPos, g.bufN);
        float sampleR = readBuf(m_bufR, g.readPos, g.bufN);
        wetL += sampleL * env * g.panL;
        wetR += sampleR * env * g.panR;

        g.readPos += g.rate;
        if (++g.age >= g.duration) g.active = false;
    }

    wetL *= m_smoothedScale * m_smoothedMix;
    wetR *= m_smoothedScale * m_smoothedMix;

    if (!m_p.freeze && N > 0)
        m_writePos = (m_writePos + 1) % N;
}

} // namespace oscilleon
} // namespace Steinberg
