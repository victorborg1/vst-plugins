// Processor.h (unchanged but included for completeness)
#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "Parameter.h"
#include "dsp/Granular.h"
#include <atomic>

namespace Steinberg {
namespace oscilleon {

class Processor : public Vst::AudioEffect {
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
    tresult PLUGIN_API setState(IBStream* state)                      override;
    tresult PLUGIN_API getState(IBStream* state)                      override;

private:
    void  buildParams();
    float denorm(ParamID id) const;
    void  sendWaveformMessage();

    static constexpr double  kMaxBufferSeconds    = 6.0;
    static constexpr int32   kSendIntervalSamples = 4096;
    static constexpr uint32  kWaveDisplayPoints   = 256;

    Granular    m_engine;

    float             m_paramValues[kParamCount] = {};
    std::atomic<bool> m_paramsChanged{ false };

    float m_dry          = 1.0f;
    float m_prevBufSec   = 3.0f;
    float m_bufSec       = 3.0f;
    int   m_effectiveBuf = 0;
    int   m_maxBufSamples = 0;

    float m_smoothedDry = 1.0f;
    float m_dryCoeff    = 0.0f;
    
    int32 m_sendTimer = 0;
};

} // namespace oscilleon
} // namespace Steinberg