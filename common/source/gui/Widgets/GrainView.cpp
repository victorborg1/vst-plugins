#include "GrainView.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace oscilleon::gui {

GrainView::GrainView() {
    ClearGrains();
}

void GrainView::ClearGrains() {
    for (auto& g : m_grains) g.active = false;
}

void GrainView::SetBufferSize(float seconds) {
    m_bufferSizeSec = std::max(seconds, 0.01f);
    m_scanOffset    = 0.f;
    ClearGrains();
    NotifyBufferCleared();
}

void GrainView::SetFreeze(bool freeze) {
    m_freeze = freeze;
}

void GrainView::NotifyBufferCleared() {
    m_flashIntensity = 1.0f;
}

void GrainView::SetWaveformData(const float* L, const float* R,
                                uint32_t count, float bufferSizeSec, uint32_t writePos)
{
    uint32_t n = std::min(count, static_cast<uint32_t>(kWaveSteps));
    std::copy(L, L + n, m_waveL);
    std::copy(R, R + n, m_waveR);
    m_bufferSizeSec        = bufferSizeSec;
    m_targetWritePosNorm   = float(writePos % kWaveSteps) / float(kWaveSteps);

    if (!m_writeHeadInitialised) {
        m_smoothWritePosNorm   = m_targetWritePosNorm;
        m_writeHeadInitialised = true;
    }
    m_hasRealWave = true;
}

float GrainView::Hann(float age, float duration) const {
    float p = std::clamp(age / std::max(duration, 1e-5f), 0.f, 1.f);
    return 0.5f * (1.f - std::cos(glm::two_pi<float>() * p));
}

float GrainView::BufPosToX(float bufPosNorm, float iX, float iW) const {
    return iX + std::clamp(bufPosNorm, 0.f, 1.f) * iW;
}

void GrainView::SpawnGrain() {
    if (m_freeze) return;

    VisualGrain* g = nullptr;
    for (auto& grain : m_grains)
        if (!grain.active) { g = &grain; break; }
    if (!g) return;

    std::uniform_real_distribution<float> sprayDist(-(m_spray * 0.20f), m_spray * 0.20f);
    std::uniform_real_distribution<float> brightDist(0.72f, 1.0f);
    std::uniform_real_distribution<float> coinFlip(0.f, 1.f);
    std::uniform_real_distribution<float> panDist(-0.3f, 0.3f);

    float delay  = std::clamp(m_smoothPosition + m_scanOffset + sprayDist(m_rng), 0.01f, 0.99f);
    float bufPos = Wrap01(m_smoothWritePosNorm - delay);

    float pitchRate = std::pow(2.f, m_pitch / 12.f);
    bool  reversed  = coinFlip(m_rng) < m_reverseProb;

    g->bufferPosNorm = bufPos;
    g->rate          = (reversed ? -pitchRate : pitchRate) / m_bufferSizeSec;
    g->duration      = std::max(m_grainSizeMs * 0.001f, 0.01f);
    g->age           = 0.f;
    g->pan           = std::clamp(m_pan + panDist(m_rng), -1.f, 1.f);
    g->brightness    = brightDist(m_rng) * m_mix;
    g->reversed      = reversed;
    g->active        = true;
}

void GrainView::Update(float dt) {
    if (m_flashIntensity > 0.f) {
        m_flashIntensity -= dt * 4.f;
        if (m_flashIntensity < 0.f) m_flashIntensity = 0.f;
    }
    if (m_hasRealWave) {
        float drift = CircleDiff(m_targetWritePosNorm, m_smoothWritePosNorm);
        m_smoothWritePosNorm = Wrap01(m_smoothWritePosNorm + drift * std::min(dt * 22.f, 1.f));
    }
    m_smoothPosition += (m_targetPosition - m_smoothPosition) * std::min(dt * 14.f, 1.f);
    if (!m_freeze) {
        m_scanOffset = Wrap01(m_scanOffset + m_increment * dt / m_bufferSizeSec);
    }
    float interval = 1.f / std::max(m_density, 0.1f);
    m_spawnTimer -= dt;
    if (m_spawnTimer <= 0.f && !m_freeze) {
        SpawnGrain();
        m_spawnTimer += interval;
        if (m_spawnTimer < 0.f) m_spawnTimer = 0.f;
    }
    for (auto& g : m_grains) {
        if (!g.active) continue;
        g.bufferPosNorm = Wrap01(g.bufferPosNorm + g.rate * dt);
        g.age += dt;
        if (g.age >= g.duration) g.active = false;
    }
}

void GrainView::Render(Renderer& renderer) {
    renderer.DrawRect({ m_x, m_y }, { m_width, m_height },
        { 0.014f, 0.016f, 0.024f, 1.0f });

    const float margin = 10.f;
    const float iX     = m_x + margin;
    const float iY     = m_y + margin;
    const float iW     = m_width  - 2.f * margin;
    const float iH     = m_height - 2.f * margin;
    const float midY   = iY + iH * 0.5f;

    if (m_hasRealWave) {
        float writeBinF = m_smoothWritePosNorm * float(kWaveSteps);

        for (int i = 0; i < kWaveSteps; ++i) {
            float x = iX + (float(i) + 0.5f) / float(kWaveSteps) * iW;

            float dist = writeBinF - float(i);
            if (dist < 0.f) dist += float(kWaveSteps);
            float age = dist / float(kWaveSteps);  

            float brightness = 0.18f + (1.f - age) * 0.82f;
            float alphaL     = 0.20f + (1.f - age) * 0.60f;
            float alphaR     = alphaL * 0.55f;

            float maxAmp = iH * 0.48f;
            float ampL   = std::min(m_waveL[i] * iH * 0.42f, maxAmp);
            float ampR   = std::min(m_waveR[i] * iH * 0.42f, maxAmp);

            glm::vec4 waveCol = {
                m_waveColor.r * brightness,
                m_waveColor.g * brightness,
                m_waveColor.b * brightness,
                alphaL
            };
            renderer.DrawLine({ x, midY - ampL }, { x, midY + ampL }, waveCol, 1.2f, 0.f);
            waveCol.a = alphaR;
            renderer.DrawLine({ x, midY - ampR }, { x, midY + ampR }, waveCol, 1.0f, 0.f);
        }

        float writeX = BufPosToX(m_smoothWritePosNorm, iX, iW);
        renderer.DrawLine({ writeX, iY }, { writeX, iY + iH },
            { m_writeHeadColor.r, m_writeHeadColor.g, m_writeHeadColor.b, 0.80f },
            1.8f, 8.f);
    }

    constexpr int kGrid = 6;
    for (int t = 1; t < kGrid; ++t) {
        float tx = iX + float(t) / float(kGrid) * iW;
        renderer.DrawLine({ tx, iY }, { tx, iY + iH },
            { m_gridColor.r, m_gridColor.g, m_gridColor.b, 0.22f }, 0.5f, 0.f);
    }
    renderer.DrawLine({ iX, midY }, { iX + iW, midY },
        { m_gridColor.r * 1.4f, m_gridColor.g * 1.4f, m_gridColor.b * 1.6f, 0.30f }, 0.6f, 0.f);

    float readPosNorm = Wrap01(m_smoothWritePosNorm - m_smoothPosition);
    float posPx       = BufPosToX(readPosNorm, iX, iW);

    float sprayHalf = m_spray * 0.20f * iW;
    float bandL     = std::max(posPx - sprayHalf, iX);
    float bandR     = std::min(posPx + sprayHalf, iX + iW);
    if (bandR - bandL > 0.5f) {
        renderer.DrawRect({ bandL, iY }, { bandR - bandL, iH },
            { m_primaryColor.r, m_primaryColor.g, m_primaryColor.b, 0.07f });
        renderer.DrawLine({ bandL, iY }, { bandL, iY + iH },
            { m_primaryColor.r, m_primaryColor.g, m_primaryColor.b, 0.14f }, 0.6f, 0.f);
        renderer.DrawLine({ bandR, iY }, { bandR, iY + iH },
            { m_primaryColor.r, m_primaryColor.g, m_primaryColor.b, 0.14f }, 0.6f, 0.f);
    }
    renderer.DrawLine({ posPx, iY }, { posPx, iY + iH },
        { m_primaryColor.r, m_primaryColor.g, m_primaryColor.b, 0.28f }, 1.0f, 0.f);

    float scanPosNorm = Wrap01(readPosNorm - m_scanOffset);
    float scanPx      = BufPosToX(scanPosNorm, iX, iW);
    renderer.DrawLine({ scanPx, iY }, { scanPx, iY + iH },
        { m_primaryColor.r, m_primaryColor.g, m_primaryColor.b, 0.58f }, 1.6f, 12.f);

    constexpr int kSegments = 32;
    float segW = iW / float(kSegments);

    float segEnergy[kSegments]{};
    bool  segReversed[kSegments]{};

    for (const auto& g : m_grains) {
        if (!g.active) continue;
        float env = Hann(g.age, g.duration);
        int   seg = int(g.bufferPosNorm * float(kSegments));
        seg = std::clamp(seg, 0, kSegments - 1);
        segEnergy[seg]  += env * g.brightness;
        if (g.reversed) segReversed[seg] = true;
    }

    for (int s = 0; s < kSegments; ++s) {
        if (segEnergy[s] < 0.01f) continue;
        float     energy = std::min(segEnergy[s], 1.f);
        float     sx     = iX + float(s) * segW;
        glm::vec4 base   = segReversed[s] ? m_accentColor : m_primaryColor;

        renderer.DrawRect({ sx + 1.f, iY }, { segW - 2.f, iH },
            { base.r, base.g, base.b, energy * 0.55f });
        renderer.DrawLine({ sx, iY }, { sx + segW, iY },
            { base.r, base.g, base.b, energy * 0.9f }, 1.5f, 0.f);
        renderer.DrawLine({ sx, iY + iH }, { sx + segW, iY + iH },
            { base.r, base.g, base.b, energy * 0.9f }, 1.5f, 0.f);
    }

    if (m_flashIntensity > 0.f)
        renderer.DrawRect({ m_x, m_y }, { m_width, m_height },
            { m_writeHeadColor.r, m_writeHeadColor.g, m_writeHeadColor.b,
              m_flashIntensity * 0.3f });

    int activeCount = 0;
    for (const auto& g : m_grains) if (g.active) ++activeCount;

    char buf[32];
    snprintf(buf, sizeof(buf), "%d grains", activeCount);
    renderer.DrawString(buf, "Labels", { iX + 4.f, iY + 4.f },
        { m_textColor.r, m_textColor.g, m_textColor.b, 0.36f }, 0.25f);
}

} // namespace oscilleon::gui
