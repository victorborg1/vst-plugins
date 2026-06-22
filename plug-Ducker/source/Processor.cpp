#include "Processor.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include <cmath>
#include <algorithm>
#include "cids.h"
#include "SharedState.h"

namespace Steinberg {
namespace oscilleon {

// ---------------------------------------------------------------------------
Processor::Processor()
{
    setControllerClass(kControllerUID);
    m_curveSamples.fill(0.0f);
}

Processor::~Processor() {}

// ---------------------------------------------------------------------------
tresult PLUGIN_API Processor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) return result;

    addAudioInput(STR16("Stereo In"),  Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);

    for (int i = 0; i < kParamCount; ++i)
        m_paramValues[i] = static_cast<float>(
            getParamInfo(static_cast<ParamID>(i)).defaultValueNormalized);

    return kResultOk;
}

tresult PLUGIN_API Processor::terminate()  { return AudioEffect::terminate(); }

tresult PLUGIN_API Processor::setActive(TBool state)
{
    m_smoothedGain = 1.0f;   // reset smoother to silence any transient on start
    return AudioEffect::setActive(state);
}

tresult PLUGIN_API Processor::setupProcessing(Vst::ProcessSetup& newSetup)
{
    tresult r = AudioEffect::setupProcessing(newSetup);
    applyParams();
    return r;
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 symbolicSampleSize)
{
    return (symbolicSampleSize == Vst::kSample32) ? kResultTrue : kResultFalse;
}

uint32 PLUGIN_API Processor::getProcessContextRequirements()
{

    return (1u << 2) | (1u << 6) | (1u << 10);
}

// ---------------------------------------------------------------------------
float Processor::denorm(ParamID id) const
{
    const auto& info = getParamInfo(id);
    return static_cast<float>(info.min + m_paramValues[id] * (info.max - info.min));
}

// ---------------------------------------------------------------------------
void Processor::applyParams()
{
    double sr = processSetup.sampleRate > 0.0 ? processSetup.sampleRate : 44100.0;

    m_amount    = m_paramValues[kParamAmount];
    m_attackMs  = denorm(kParamAttack);
    m_releaseMs = denorm(kParamRelease);
    m_mix       = m_paramValues[kParamMix];

    // One-pole coefficient: α = 1 − exp(−1 / (sr * T))
    auto msToCoeff = [sr](float ms) -> float {
        if (ms <= 0.0f) return 1.0f;
        return 1.0f - std::exp(-1.0f / (static_cast<float>(sr) * ms * 0.001f));
    };

    m_attackCoeff  = msToCoeff(m_attackMs);
    m_releaseCoeff = msToCoeff(m_releaseMs);
}

// ---------------------------------------------------------------------------
float Processor::sampleCurve(float phase) const
{
    phase = std::clamp(phase, 0.0f, 1.0f);
    float idx  = phase * static_cast<float>(::oscilleon::kCurveSampleCount - 1);
    int   i0   = static_cast<int>(idx);
    int   i1   = std::min(i0 + 1, ::oscilleon::kCurveSampleCount - 1);
    float frac = idx - static_cast<float>(i0);
    return m_curveSamples[i0] * (1.0f - frac) + m_curveSamples[i1] * frac;
}

// ---------------------------------------------------------------------------
tresult PLUGIN_API Processor::setState(IBStream* state)
{
    IBStreamer s(state, kLittleEndian);

    // Params
    for (int i = 0; i < kParamCount; ++i) {
        float v = 0.f;
        if (!s.readFloat(v)) return kResultFalse;
        m_paramValues[i] = v;
    }

    // Curve
    for (int i = 0; i < ::oscilleon::kCurveSampleCount; ++i) {
        float v = 0.f;
        if (!s.readFloat(v)) break;
        m_curveSamples[i] = v;
    }

    m_paramsChanged = true;
    return kResultOk;
}

tresult PLUGIN_API Processor::getState(IBStream* state)
{
    IBStreamer s(state, kLittleEndian);

    for (int i = 0; i < kParamCount; ++i)
        if (!s.writeFloat(m_paramValues[i])) return kResultFalse;

    for (int i = 0; i < ::oscilleon::kCurveSampleCount; ++i)
        if (!s.writeFloat(m_curveSamples[i])) return kResultFalse;

    return kResultOk;
}


tresult PLUGIN_API Processor::process(Vst::ProcessData& data)
{
    if (data.numSamples == 0) return kResultOk;


    if (data.inputParameterChanges) {
        int32 numParams = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < numParams; ++i) {
            auto* queue = data.inputParameterChanges->getParameterData(i);
            if (!queue) continue;
            Vst::ParamID id = queue->getParameterId();
            if (id >= static_cast<Vst::ParamID>(kParamCount)) continue;
            int32 np = queue->getPointCount();
            if (np <= 0) continue;
            Vst::ParamValue val; int32 offset;
            if (queue->getPoint(np - 1, offset, val) == kResultOk) {
                m_paramValues[id] = static_cast<float>(val);
                m_paramsChanged   = true;
            }
        }
    }

    if (m_paramsChanged.exchange(false)) applyParams();


    ::oscilleon::SharedLfoState::instance().tryUpdateCurve(m_curveSamples);


    float* inL  = (data.inputs  && data.inputs[0].numChannels  > 0)
                    ? static_cast<float*>(data.inputs[0].channelBuffers32[0])  : nullptr;
    float* inR  = (data.inputs  && data.inputs[0].numChannels  > 1)
                    ? static_cast<float*>(data.inputs[0].channelBuffers32[1])  : nullptr;
    float* outL = (data.outputs && data.outputs[0].numChannels > 0)
                    ? static_cast<float*>(data.outputs[0].channelBuffers32[0]) : nullptr;
    float* outR = (data.outputs && data.outputs[0].numChannels > 1)
                    ? static_cast<float*>(data.outputs[0].channelBuffers32[1]) : nullptr;
    if (!outL || !outR) return kResultOk;


    bool   isPlaying    = false;
    double beatPos      = 0.0;  
    double beatsPerSample = 0.0;

    if (data.processContext) {
        const auto& ctx = *data.processContext;
        isPlaying = (ctx.state & Vst::ProcessContext::kPlaying) != 0;

        if (ctx.state & Vst::ProcessContext::kProjectTimeMusicValid)
            beatPos = ctx.projectTimeMusic;

        if ((ctx.state & Vst::ProcessContext::kTempoValid) && isPlaying)
            beatsPerSample = ctx.tempo / (60.0 * processSetup.sampleRate);
    }


    float phase = static_cast<float>(std::fmod(beatPos, 1.0));
    if (phase < 0.0f) phase += 1.0f;


    for (int32 i = 0; i < data.numSamples; ++i)
    {
        const float dryL = inL ? inL[i] : 0.f;
        const float dryR = inR ? inR[i] : 0.f;

        const float lfoValue   = sampleCurve(phase);
        const float targetGain = std::clamp(1.0f - m_amount * lfoValue, 0.0f, 1.0f);


        const float coeff = (targetGain < m_smoothedGain) ? m_attackCoeff : m_releaseCoeff;
        m_smoothedGain   += coeff * (targetGain - m_smoothedGain);


        const float wetL = dryL * m_smoothedGain;
        const float wetR = dryR * m_smoothedGain;

        outL[i] = m_mix * wetL + (1.0f - m_mix) * dryL;
        outR[i] = m_mix * wetR + (1.0f - m_mix) * dryR;


        if (isPlaying) {
            phase += static_cast<float>(beatsPerSample);
            if (phase >= 1.0f) phase -= std::floor(phase);
        }
    }

    ::oscilleon::SharedLfoState::instance().setIsPlaying(isPlaying);
    ::oscilleon::SharedLfoState::instance().setPhase(phase);
    return kResultOk;
}

} // namespace oscilleon
} // namespace Steinberg