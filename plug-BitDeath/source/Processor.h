#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "Parameter.h"
#include "dsp/util.h"
#include "dsp/smoother.h"
#include "dsp/Tempo.h"
#include <atomic>
#include <random>

namespace Steinberg {
namespace oscilleon {

static constexpr float kRandRates[] = {
    1000.f, 2000.f, 3000.f, 4000.f, 6000.f,
    8000.f, 11025.f, 16000.f, 22050.f
};
static constexpr int kRandRateCount = 9;

class Processor : public Vst::AudioEffect {
public:
    Processor();
    ~Processor() override;
    static FUnknown* createInstance(void*) { return (Vst::IAudioProcessor*)new Processor; }

    tresult PLUGIN_API initialize(FUnknown* context)                  override;
    tresult PLUGIN_API terminate()                                    override;
    tresult PLUGIN_API setActive(TBool state)                         override;
    tresult PLUGIN_API setupProcessing(Vst::ProcessSetup& newSetup)   override;
    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;
    tresult PLUGIN_API process(Vst::ProcessData& data)                override;
    tresult PLUGIN_API setState(IBStream* state)                      override;
    tresult PLUGIN_API getState(IBStream* state)                      override;
    //uint32 PLUGIN_API getProcessContextRequirements()                 override;

private:
    void  buildParams();
    float denorm(ParamID id) const;
    void  applyRandomRate();
    float randomRate();

    float m_paramValues[kParamCount] = {};
    std::atomic<bool> m_paramsChanged{ false };


    double sampleRate = 44100.0;
    dsp::util::Downsampler<float> m_downsamplerL;
    dsp::util::Downsampler<float> m_downsamplerR;

    dsp::smoother::OnePole m_bitSmoother;
    dsp::smoother::OnePole m_mixSmoother;

    bool                 m_randActive = false;
    Tempo  m_randomizer;
    std::mt19937 m_rng{ std::random_device{}() };
};

} // namespace oscilleon
} // namespace Steinberg
