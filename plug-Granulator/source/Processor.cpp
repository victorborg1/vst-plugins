// Processor.cpp (unchanged from previous)
#include "Processor.h"
#include "cids.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace Steinberg {
namespace oscilleon {

Processor::Processor() { setControllerClass(kControllerUID); }
Processor::~Processor() {}

tresult PLUGIN_API Processor::initialize(FUnknown* context) {
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) return result;
    addAudioInput(STR16("Stereo In"),   Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);
    for (int i = 0; i < kParamCount; ++i)
        m_paramValues[i] = static_cast<float>(getParamInfo(static_cast<ParamID>(i)).defaultValueNormalized);
    return kResultOk;
}

tresult PLUGIN_API Processor::terminate()                       { return AudioEffect::terminate(); }
tresult PLUGIN_API Processor::setActive(TBool state)           { return AudioEffect::setActive(state); }

tresult PLUGIN_API Processor::setupProcessing(Vst::ProcessSetup& newSetup) {
    tresult r = AudioEffect::setupProcessing(newSetup);
    double sr    = processSetup.sampleRate > 0.0 ? processSetup.sampleRate : 44100.0;
    m_maxBufSamples = static_cast<int>(sr * kMaxBufferSeconds);
    m_engine.prepare(sr, m_maxBufSamples);

    m_dryCoeff = 1.0f - static_cast<float>(std::exp(-1.0 / (sr * 0.005)));

    buildParams();
    return r;
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 symbolicSampleSize) {
    return (symbolicSampleSize == Vst::kSample32) ? kResultTrue : kResultFalse;
}

float Processor::denorm(ParamID id) const {
    const auto& info = getParamInfo(id);
    return static_cast<float>(info.min + m_paramValues[id] * (info.max - info.min));
}

void Processor::buildParams() {
    double sr = processSetup.sampleRate > 0.0 ? processSetup.sampleRate : 44100.0;

    m_dry = m_paramValues[kParamDry];

    float newBufSec  = denorm(kParamBufferSize);
    m_bufSec         = newBufSec;
    m_prevBufSec     = newBufSec;
    int newEffectiveBuf = std::clamp(static_cast<int>(m_bufSec * sr), 1, m_maxBufSamples);
    if (newEffectiveBuf != m_effectiveBuf) {
        m_effectiveBuf = newEffectiveBuf;
        m_engine.setEffectiveBuf(m_effectiveBuf);
    }

    GranularParams p;
    p.freeze             = m_paramValues[kParamFreeze] >= 0.5f;
    p.inputGain          = m_paramValues[kParamInput];
    p.feedback           = std::clamp(m_paramValues[kParamFeedback], 0.f, 0.95f);
    p.effectiveBuf       = m_effectiveBuf;
    p.maxGrains          = std::max(1, static_cast<int>(std::round(denorm(kParamMaxGrains))));
    p.position           = m_paramValues[kParamPosition];
    p.increment          = denorm(kParamIncrement);
    p.grainInterval      = sr / static_cast<double>(std::max(0.01f, denorm(kParamRate)));
    p.grainDuration      = std::max(2, static_cast<int>(denorm(kParamLength) * 0.001 * sr));
    p.pitchRate          = std::pow(2.0, static_cast<double>(std::round(denorm(kParamPitch))) / 12.0);
    p.reverseProbability = m_paramValues[kParamReverse];
    p.pan                = denorm(kParamPan);
    p.volume             = m_paramValues[kParamVolume];
    p.granulatorMix      = m_paramValues[kParamGranulator];

    m_engine.setParams(p);
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

void Processor::sendWaveformMessage() {
    const auto& bufL = m_engine.getBufL();
    const auto& bufR = m_engine.getBufR();
    if (m_effectiveBuf <= 0 || bufL.empty()) return;

    auto* msg = allocateMessage();
    if (!msg) return;
    msg->setMessageID("waveform");
    auto* attr = msg->getAttributes();

    static constexpr int N = static_cast<int>(kWaveDisplayPoints);
    float peakL[kWaveDisplayPoints]{};
    float peakR[kWaveDisplayPoints]{};

    for (int bin = 0; bin < N; ++bin) {
        int start = (bin * m_effectiveBuf) / N;
        int end   = std::max(((bin + 1) * m_effectiveBuf) / N, start + 1);
        for (int s = start; s < end; ++s) {
            peakL[bin] = std::max(peakL[bin], std::abs(bufL[s]));
            peakR[bin] = std::max(peakR[bin], std::abs(bufR[s]));
        }
    }

    uint32 writeBin = static_cast<uint32>(
        (static_cast<int64>(m_engine.getWritePos()) * N) / std::max(m_effectiveBuf, 1));

    attr->setBinary("L",        peakL,      sizeof(peakL));
    attr->setBinary("R",        peakR,      sizeof(peakR));
    attr->setFloat ("bufSize",  m_bufSec);
    attr->setInt   ("writePos", writeBin);

    sendMessage(msg);
    msg->release();
}

tresult PLUGIN_API Processor::process(Vst::ProcessData& data) {
    if (data.numSamples == 0) return kResultOk;

    if (data.inputParameterChanges) {
        int32 count = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < count; ++i) {
            auto* q = data.inputParameterChanges->getParameterData(i);
            if (!q) continue;
            Vst::ParamID id = q->getParameterId();
            if (id >= static_cast<Vst::ParamID>(kParamCount)) continue;
            Vst::ParamValue val; 
            int32 offset;
            if (q->getPoint(q->getPointCount() - 1, offset, val) == kResultOk) {
                m_paramValues[id] = static_cast<float>(val);
                m_paramsChanged   = true;
            }
        }
    }

    if (m_paramsChanged.exchange(false))
        buildParams();

    float* inL  = (data.inputs  && data.inputs[0].numChannels  > 0) ? static_cast<float*>(data.inputs[0].channelBuffers32[0])  : nullptr;
    float* inR  = (data.inputs  && data.inputs[0].numChannels  > 1) ? static_cast<float*>(data.inputs[0].channelBuffers32[1])  : nullptr;
    float* outL = (data.outputs && data.outputs[0].numChannels > 0) ? static_cast<float*>(data.outputs[0].channelBuffers32[0]) : nullptr;
    float* outR = (data.outputs && data.outputs[0].numChannels > 1) ? static_cast<float*>(data.outputs[0].channelBuffers32[1]) : nullptr;
    if (!outL || !outR) return kResultOk;

    for (int32 i = 0; i < data.numSamples; ++i) {
        float dryL = inL ? inL[i] : 0.f;
        float dryR = inR ? inR[i] : 0.f;
        float wetL, wetR;
        m_engine.processSample(dryL, dryR, wetL, wetR);

        m_smoothedDry += m_dryCoeff * (m_dry - m_smoothedDry);

        outL[i] = dryL * m_smoothedDry + wetL;
        outR[i] = dryR * m_smoothedDry + wetR;
    }

    m_sendTimer -= data.numSamples;
    if (m_sendTimer <= 0) {
        m_sendTimer = kSendIntervalSamples;
        sendWaveformMessage();
    }

    return kResultOk;
}

} // namespace oscilleon
} // namespace Steinberg