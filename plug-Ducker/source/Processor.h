#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "Parameter.h"
#include "SharedState.h"
#include <atomic>
#include <array>

namespace Steinberg {
namespace oscilleon {

    
class Processor : public Vst::AudioEffect
{
public:
    Processor();
    ~Processor() override;
    static FUnknown* createInstance(void*) { return (Vst::IAudioProcessor*)new Processor; }

    tresult PLUGIN_API initialize(FUnknown* context)                  override;
    tresult PLUGIN_API terminate()                                     override;
    tresult PLUGIN_API setActive(TBool state)                         override;
    tresult PLUGIN_API setupProcessing(Vst::ProcessSetup& newSetup)   override;
    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;
    tresult PLUGIN_API process(Vst::ProcessData& data)                override;
    tresult PLUGIN_API setState(IBStream* state)                       override;
    tresult PLUGIN_API getState(IBStream* state)                       override;


    uint32 PLUGIN_API getProcessContextRequirements()                  override;

private:
    void  applyParams();
    float denorm(ParamID id) const;
    float sampleCurve(float phase) const;  


    float m_paramValues[kParamCount] = {};
    std::atomic<bool> m_paramsChanged{ false };

    float m_amount    = 1.0f; 
    float m_attackMs  = 5.0f;
    float m_releaseMs = 50.0f; 
    float m_mix       = 1.0f; 


    float m_smoothedGain  = 1.0f;
    float m_attackCoeff   = 0.0f;
    float m_releaseCoeff  = 0.0f;   


    std::array<float, ::oscilleon::kCurveSampleCount> m_curveSamples{};
};

} // namespace oscilleon
} // namespace Steinberg