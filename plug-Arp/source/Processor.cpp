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
    addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);
    for (int i = 0; i < kParamCount; ++i)
        m_paramValues[i] = static_cast<float>(getParamInfo(static_cast<ParamID>(i)).defaultValueNormalized);
    return kResultOk;
}

tresult PLUGIN_API Processor::terminate()        { return AudioEffect::terminate(); }
tresult PLUGIN_API Processor::setActive(TBool s) { return AudioEffect::setActive(s); }

tresult PLUGIN_API Processor::setupProcessing(Vst::ProcessSetup& newSetup) {
    tresult r = AudioEffect::setupProcessing(newSetup);
    sampleRate = newSetup.sampleRate;
    buildParams();
    return r;
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 sz) {
    return sz == Vst::kSample32 ? kResultTrue : kResultFalse;
}

float Processor::denorm(ParamID id) const {
    const auto& info = getParamInfo(id);
    return static_cast<float>(info.min + m_paramValues[id] * (info.max - info.min));
}

void Processor::buildParams() {
    // TODO: read m_paramValues / denorm() and configure your DSP here
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


    //events
    Vst::IEventList* events = data.inputEvents;
    if (events != nullptr) {
        int32 numEvents = events->getEventCount();
        for (int32 i = 0; i < numEvents; ++i) {
            Vst::Event event;
            if (events->getEvent(i, event) == kResultOk) {
                switch (event.type) {
                case Vst::Event::kNoteOnEvent:
                {
                    int note = event.noteOn.pitch;
                    frequency = 440.0 * pow(2.0, (note - 69) / 12.0);
                    break;
                }
                }
            }
        }
    }


    float* outL = (data.outputs && data.outputs[0].numChannels > 0)
        ? (float*)data.outputs[0].channelBuffers32[0] : nullptr;

    float* outR = (data.outputs && data.outputs[0].numChannels > 1)
        ? (float*)data.outputs[0].channelBuffers32[1] : nullptr;

    if (!outL || !outR) return kResultOk;

    const double phaseInc = 2.0 * pi * frequency / sampleRate;

    for (int32 i = 0; i < data.numSamples; ++i) {
        float sample = static_cast<float>(std::sin(phase));

        phase += phaseInc;

        // wrap phase
        if (phase >= 2.0 * pi)
            phase -= 2.0 * pi;

        outL[i] = sample * 0.0f; // keep volume low
        outR[i] = sample * 0.0f;
    }

    return kResultOk;
}

} // namespace oscilleon
} // namespace Steinberg