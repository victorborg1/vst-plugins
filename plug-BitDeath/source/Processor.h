#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "Parameter.h"
#include "dsp/util.h"
#include "dsp/smoother.h"
#include <atomic>

namespace Steinberg {
namespace oscilleon {

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

private:
    void  buildParams();
    float denorm(ParamID id) const;

    float m_paramValues[kParamCount] = {};
    std::atomic<bool> m_paramsChanged{ false };


    double sampleRate = 44100.0;
    dsp::util::Downsampler<float> m_downsamplerL;
    dsp::util::Downsampler<float> m_downsamplerR;

    dsp::smoother::OnePole m_bitSmoother;
    dsp::smoother::OnePole m_mixSmoother;
};

} // namespace oscilleon
} // namespace Steinberg