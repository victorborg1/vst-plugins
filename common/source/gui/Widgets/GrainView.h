#pragma once
#include "Widget.h"
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
 
namespace oscilleon::gui {
 
struct VisualGrain {
    float bufferPosNorm = 0.f;
    float rate          = 1.f;   // normalised buffer advance per second (neg = reversed)
    float age           = 0.f;
    float duration      = 0.f;
    float pan           = 0.f;
    float brightness    = 1.f;
    bool  reversed      = false;
    bool  active        = false;
};
 
class GrainView : public Widget {
public:
    GrainView();
 
    void Update(float dt);
    void Render(Renderer& renderer) override;
 
    void SetGrainSize(float ms)  { m_grainSizeMs    = ms;  }
    void SetDensity(float d)     { m_density        = d;   }
    void SetPosition(float p)    { m_targetPosition = p;   }
    void SetIncrement(float inc) { m_increment      = inc; }
    void SetSpray(float s)       { m_spray          = s;   }
    void SetPitch(float semi)    { m_pitch          = semi; }
    void SetReverse(float prob)  { m_reverseProb    = prob; }
    void SetPan(float pan)       { m_pan            = pan;  }
    void SetMix(float m)         { m_mix            = m;    }
 
    void SetBufferSize(float seconds);
    void SetFreeze(bool freeze);
    void NotifyBufferCleared();
 
    // writePos is in [0, kWaveSteps) bin space, matching kWaveDisplayPoints in the processor
    void SetWaveformData(const float* L, const float* R,
                         uint32_t count, float bufferSizeSec, uint32_t writePos);
 
    void SetPrimaryColor(const glm::vec4& c)   { m_primaryColor   = c; }
    void SetAccentColor(const glm::vec4& c)    { m_accentColor    = c; }
    void SetWaveColor(const glm::vec4& c)      { m_waveColor      = c; }
    void SetWriteHeadColor(const glm::vec4& c) { m_writeHeadColor = c; }
    void SetGridColor(const glm::vec4& c)      { m_gridColor      = c; }
    void SetTextColor(const glm::vec4& c)      { m_textColor      = c; }
 
private:
    void  SpawnGrain();
    void  ClearGrains();
    float Hann(float age, float duration) const;
    float BufPosToX(float bufPosNorm, float iX, float iW) const;
 
    static float Wrap01(float v) {
        v = std::fmod(v, 1.f);
        return v < 0.f ? v + 1.f : v;
    }
 
    // Shortest signed arc on [0,1) circle, result in [-0.5, 0.5]
    static float CircleDiff(float target, float current) {
        float d = target - current;
        if (d >  0.5f) d -= 1.f;
        if (d < -0.5f) d += 1.f;
        return d;
    }
 
    float m_grainSizeMs  = 100.f;
    float m_density      = 8.f;
 
    float m_targetPosition = 0.f;   // raw value from SetPosition()
    float m_smoothPosition = 0.f;   // lerped in Update(), used for rendering
 
    // Scan offset advances continuously every frame.
    //   m_increment > 0  → scan head moves RIGHT → LEFT  (deeper into the past)
    //   m_increment < 0  → scan head moves LEFT  → RIGHT (toward the write head)
    //   m_increment = 0  → stationary
    float m_scanOffset  = 0.f;
    float m_increment   = 0.f;
 
    float m_spray        = 0.f;
    float m_pitch        = 0.f;
    float m_reverseProb  = 0.f;
    float m_pan          = 0.f;
    float m_mix          = 0.5f;
    float m_bufferSizeSec= 3.0f;
    bool  m_freeze       = false;
 
    float m_flashIntensity = 0.f;
    float m_spawnTimer     = 0.f;
 
    static constexpr int kMaxGrains = 48;
    static constexpr int kWaveSteps = 256;
 
    float m_waveL[kWaveSteps]{};
    float m_waveR[kWaveSteps]{};
 
    // Write head:
    //   m_targetWritePosNorm  — ground truth from the processor, updated ~every 93 ms
    //   m_smoothWritePosNorm  — fast lerp toward target; used for both the line AND
    //                           the waveform age so they are always in perfect sync.
    //                           No dead-reckoning so the head never overshoots fresh data.
    float m_smoothWritePosNorm   = 0.f;
    float m_targetWritePosNorm   = 0.f;
    bool  m_writeHeadInitialised = false;
    bool  m_hasRealWave          = false;
 
    VisualGrain  m_grains[kMaxGrains]{};
    std::mt19937 m_rng{ std::random_device{}() };
 
    glm::vec4 m_primaryColor   { 0.35f, 0.60f, 1.0f,  1.0f };
    glm::vec4 m_accentColor    { 1.0f,  0.45f, 0.10f, 1.0f };
    glm::vec4 m_waveColor      { 0.30f, 0.64f, 1.0f,  1.0f };
    glm::vec4 m_writeHeadColor { 1.0f,  0.68f, 0.18f, 1.0f };
    glm::vec4 m_gridColor      { 0.06f, 0.08f, 0.13f, 1.0f };
    glm::vec4 m_textColor      { 0.26f, 0.50f, 0.84f, 1.0f };
};
 
} // namespace oscilleon::gui