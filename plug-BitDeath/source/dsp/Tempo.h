#pragma once
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include <random>
#include <functional>

namespace Steinberg {
namespace oscilleon {

static constexpr double kDivisions[] = { 0.125, 0.25, 0.5, 1.0, 2.0, 4.0 };
static constexpr int kDivCount = 6;

class Tempo {
public:
    void tick(const Vst::ProcessContext* ctx, int divIdx, std::function<void()> onTrigger);
    void reset();

private:
    double m_lastSlot   = -1.0;
    bool   m_wasPlaying = false;
};

} // namespace oscilleon
} // namespace Steinberg