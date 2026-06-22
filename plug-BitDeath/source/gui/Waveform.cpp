#include "Waveform.h"
#include "Renderer/Renderer.h"
#include <cmath>
#include <algorithm>
#include <vector>

namespace oscilleon::gui {

void Waveform::SetParams(float targetSampleRate, float bitDepth, float mix) {
    float ref = 44100.0f;
    m_downsampleSteps = std::max(1.0f, std::round(ref / std::max(1.0f, targetSampleRate)));
    m_bitDepth = std::clamp(bitDepth, 1.0f, 24.0f);
    m_mix = std::clamp(mix, 0.0f, 1.0f);
}

float Waveform::quantize(float x, int bits) const {
    bits = std::clamp(bits, 1, 24);
    float levels = static_cast<float>(1 << bits);
    x = std::clamp(x, -1.0f, 1.0f);
    float u = x * 0.5f + 0.5f;
    u = std::round(u * (levels - 1.0f)) / (levels - 1.0f);
    return u * 2.0f - 1.0f;
}

void Waveform::Render(Renderer& renderer) {
    renderer.DrawRect({ m_x, m_y }, { m_width, m_height },
                      { 0.10f, 0.10f, 0.10f, 1.0f });

    constexpr int   kSamples = 512;
    constexpr float kTwoPi  = 6.28318530717958f;

    float cx    = m_x;
    float cy    = m_y + m_height * 0.5f;
    float yHalf = m_height * 0.42f;
    float xStep = m_width / static_cast<float>(kSamples - 1);

    renderer.DrawLine({ m_x, cy }, { m_x + m_width, cy },
                      { 0.22f, 0.22f, 0.22f, 1.0f }, 1.0f);
    for (float f : { 0.25f, 0.75f }) {
        float gy = m_y + m_height * f;
        renderer.DrawLine({ m_x, gy }, { m_x + m_width, gy },
                          { 0.16f, 0.16f, 0.16f, 1.0f }, 0.5f);
    }

    std::vector<float> sine(kSamples);
    for (int i = 0; i < kSamples; ++i)
        sine[i] = std::sin(kTwoPi * static_cast<float>(i) / static_cast<float>(kSamples - 1));

    for (int i = 1; i < kSamples; ++i) {
        glm::vec2 a{ cx + (i - 1) * xStep, cy - sine[i - 1] * yHalf };
        glm::vec2 b{ cx + i       * xStep, cy - sine[i]     * yHalf };
        renderer.DrawLine(a, b, { 0.35f, 0.35f, 0.35f, 1.0f }, 1.0f);
    }

    int   step = std::max(1, static_cast<int>(std::floor(m_downsampleSteps)));
    int   bits = std::max(1, static_cast<int>(std::floor(m_bitDepth)));
    float held = 0.0f;
    int   counter = 0;

    std::vector<float> processed(kSamples);
    for (int i = 0; i < kSamples; ++i) {
        if (++counter >= step) {
            counter = 0;
            held = sine[i];
        }
        float wet = quantize(held, bits);
        processed[i] = sine[i] + m_mix * (wet - sine[i]);
    }

    counter = 0;
    int segStart = 0;
    for (int i = 1; i <= kSamples; ++i) {
        bool flush   = (i == kSamples);
        bool newStep = !flush && (counter + 1 >= step);

        if (newStep || flush) {
            float x0 = cx + segStart * xStep;
            float x1 = cx + (i - 1)  * xStep;
            float y0 = cy - processed[segStart] * yHalf;
            renderer.DrawLine({ x0, y0 }, { x1, y0 },
                              { 0.92f, 0.92f, 0.95f, 0.95f }, 1.5f);

            if (!flush && i < kSamples) {
                float y1 = cy - processed[i] * yHalf;
                renderer.DrawLine({ x1, y0 }, { x1, y1 },
                                  { 0.92f, 0.92f, 0.95f, 0.45f }, 1.0f);
            }
            segStart = i;
            counter  = 0;
        } else {
            ++counter;
        }
    }
}

} // namespace oscilleon::gui