#include "Processor.h"
#include "cids.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <public.sdk/source/vst/hosting/eventlist.h>
#include <algorithm>
#include <cmath>
#define pi 3.14159265358979

namespace Steinberg {
namespace oscilleon {

Processor::Processor()  { setControllerClass(kControllerUID); }
Processor::~Processor() {}

tresult PLUGIN_API Processor::initialize(FUnknown* context) {
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) return result;

    addAudioInput(STR16("Stereo In"), Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);
    
    for (int i = 0; i < kParamCount; ++i)
        m_paramValues[i] = static_cast<float>(getParamInfo(static_cast<ParamID>(i)).defaultValueNormalized);

    return kResultOk;
}

tresult PLUGIN_API Processor::terminate()        { return AudioEffect::terminate(); }
tresult PLUGIN_API Processor::setActive(TBool s) { return AudioEffect::setActive(s); }

uint32 PLUGIN_API Processor::getProcessContextRequirements() {
    return Vst::IProcessContextRequirements::kNeedTempo
         | Vst::IProcessContextRequirements::kNeedProjectTimeMusic;
}

tresult PLUGIN_API Processor::setupProcessing(Vst::ProcessSetup& newSetup) {
    tresult r = AudioEffect::setupProcessing(newSetup);
    sampleRate = newSetup.sampleRate;

    m_downsamplerL.reset(0.f);
    m_downsamplerR.reset(0.f);
    m_bitSmoother.reset(24.0f);
    m_mixSmoother.reset(1.0f); 

    buildParams();
    return r;
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 sz) {
    return sz == Vst::kSample32 ? kResultTrue : kResultFalse;
}

float Processor::denorm(ParamID id) const {
    const auto& info = getParamInfo(id);
    return static_cast<float>(
        info.min + m_paramValues[id] * (info.max - info.min)
    );
}

void Processor::buildParams() {
    // downsample factor
    if (!m_randActive) {
        float targetRate = denorm(kParamDownsample);
        int step = std::max(1, static_cast<int>(std::round(sampleRate / targetRate)));
        m_downsamplerL.setStep(step);
        m_downsamplerR.setStep(step);
    }

    // bit depth
    float targetBits = denorm(kParamBitDepth);
    m_bitSmoother.setTarget(targetBits);

    // mix
    float targetMix = denorm(kParamMix);
    m_mixSmoother.setTarget(targetMix);

    m_randActive = (m_paramValues[kParamRandomize] >= 0.5f);
}

float Processor::randomRate() {
    std::uniform_int_distribution<int> dist(0, kRandRateCount - 1);
    return kRandRates[dist(m_rng)];
}

void Processor::applyRandomRate() {
    float rate = randomRate();
    int step = std::max(1, static_cast<int>(std::round(sampleRate / rate)));
    m_downsamplerL.setStep(step);
    m_downsamplerR.setStep(step);
}

tresult PLUGIN_API Processor::setState(IBStream* state) {
    IBStreamer s(state, kLittleEndian);
    for (int i = 0; i < kParamCount; ++i) {
        float v = 0.f;
        if (!s.readFloat(v)) return kResultFalse;
        m_paramValues[i] = v;
    }
    m_paramsChanged = true;
    return kResultOk;
}

tresult PLUGIN_API Processor::getState(IBStream* state) {
    IBStreamer s(state, kLittleEndian);
    for (int i = 0; i < kParamCount; ++i)
        if (!s.writeFloat(m_paramValues[i])) return kResultFalse;
    return kResultOk;
}

tresult PLUGIN_API Processor::process(Vst::ProcessData& data) {
    if (data.numSamples == 0) return kResultOk;

    //notify params
    if (data.inputParameterChanges) {
        int32 count = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < count; ++i) {
            auto* q = data.inputParameterChanges->getParameterData(i);
            if (!q) continue;
            Vst::ParamID id = q->getParameterId();
            if (id >= static_cast<Vst::ParamID>(kParamCount)) continue;
            Vst::ParamValue val; int32 offset;
            if (q->getPoint(q->getPointCount() - 1, offset, val) == kResultOk) {
                m_paramValues[id] = static_cast<float>(val);
                m_paramsChanged   = true;
            }
        }
    }
    if (m_paramsChanged.exchange(false)) buildParams();

    if (m_randActive) {
        int divIdx = static_cast<int>(std::round(denorm(kParamStepDiv)));
        m_randomizer.tick(data.processContext, divIdx, [this]() {
            applyRandomRate();
        });
    }
    
    //audio inpuut
    float* inL = (data.inputs  && data.inputs[0].numChannels > 0)
                  ? (float*)data.inputs[0].channelBuffers32[0] : nullptr;
    float* inR = (data.inputs  && data.inputs[0].numChannels > 1)
                  ? (float*)data.inputs[0].channelBuffers32[1] : nullptr;
    float* outL = (data.outputs && data.outputs[0].numChannels > 0)
                  ? (float*)data.outputs[0].channelBuffers32[0] : nullptr;
    float* outR = (data.outputs && data.outputs[0].numChannels > 1)
                  ? (float*)data.outputs[0].channelBuffers32[1] : nullptr;

    if (!outL || !outR)
        return kResultOk;

    // output nothing if no inpt
    if (!inL) inL = outL;
    if (!inR) inR = outR;

    for (int32 i = 0; i < data.numSamples; ++i) {
        float dryL = inL[i];
        float dryR = inR[i];

        // downsample
        float downL = m_downsamplerL.process(dryL);
        float downR = m_downsamplerR.process(dryR);

        // smooth bit depth and convert to integer
        float smoothBits = m_bitSmoother.process();
        int   bits       = std::clamp(static_cast<int>(std::floor(smoothBits)), 1, 24);

        // quantize
        float wetL = dsp::util::quantize(downL, bits);
        float wetR = dsp::util::quantize(downR, bits);

        // smooth mix coefficient
        float mix = m_mixSmoother.process();

        // dry/wet mix
        outL[i] = dryL + mix * (wetL - dryL);
        outR[i] = dryR + mix * (wetR - dryR);
    }

    return kResultOk;
}

} // namespace oscilleon
} // namespace Steinberg