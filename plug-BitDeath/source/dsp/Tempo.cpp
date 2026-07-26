#include "Tempo.h"
#include <cmath>
#include <algorithm>

namespace Steinberg {
namespace oscilleon {

void Tempo::reset() {
    m_lastSlot   = -1.0;
    m_wasPlaying = false;
}

void Tempo::tick(const Vst::ProcessContext* ctx, int divIdx,
                               std::function<void()> onTrigger)
{
    if (!ctx) return;

    constexpr uint32 kNeeded = Vst::ProcessContext::kTempoValid
                             | Vst::ProcessContext::kProjectTimeMusicValid;
    if ((ctx->state & kNeeded) != kNeeded) return;

    bool isPlaying = (ctx->state & Vst::ProcessContext::kPlaying) != 0;

    if (!isPlaying) {
        reset();
        return;
    }

    double pos = ctx->projectTimeMusic;

    divIdx = std::clamp(divIdx, 0, kDivCount - 1);
    double divQN    = kDivisions[divIdx];
    double currentSlot = std::floor(pos / divQN);

    if (m_wasPlaying && currentSlot < m_lastSlot) {
        m_lastSlot = -1.0;
    }

    if (currentSlot > m_lastSlot) {
        m_lastSlot = currentSlot;
        onTrigger();
    }

    m_wasPlaying = isPlaying;
}

} // namespace oscilleon
} // namespace Steinberg