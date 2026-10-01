#include "Processor.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include <algorithm>
#include <cmath>
#include "cids.h"

namespace Steinberg {
namespace oscilleon {


Processor::Processor()
{
    setControllerClass(kControllerUID);
    heldChord = { 60, 64, 67 };
    rhythmCounters.resize(heldChord.size(), 0);
    noteOnFlags.resize(heldChord.size(), false);
}

Processor::~Processor() {}


tresult PLUGIN_API Processor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) return result;

    addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);
    addEventOutput(STR16("MIDI Out"), 1);
    addEventInput(STR16("MIDI In"), 1);

    for (int i = 0; i < kParamCount; ++i)
        paramValues[i] = static_cast<float>(getParamInfo(static_cast<ParamID>(i)).defaultValueNormalized);

    
    updateRhythmPatterns();

    return kResultOk;
}

tresult PLUGIN_API Processor::terminate() { return AudioEffect::terminate(); }
tresult PLUGIN_API Processor::setActive(TBool state) { return AudioEffect::setActive(state); }
tresult PLUGIN_API Processor::setupProcessing(Vst::ProcessSetup& newSetup)
{
    tresult r = AudioEffect::setupProcessing(newSetup);
    if (r != kResultOk) return r;
    paramsChanged = true;
    return r;
}
tresult PLUGIN_API Processor::canProcessSampleSize(int32 symbolicSampleSize)
{
    return (symbolicSampleSize == Vst::kSample32) ? kResultTrue : kResultFalse;
}


float Processor::denorm(ParamID id) const
{
    const auto& info = getParamInfo(id);
    return static_cast<float>(info.min + paramValues[id] * (info.max - info.min));
}


tresult PLUGIN_API Processor::setState(IBStream* state)
{
    IBStreamer s(state, kLittleEndian);
    for (int i = 0; i < kParamCount; ++i)
    {
        float v = 0.f;
        if (!s.readFloat(v)) return kResultFalse;
        paramValues[i] = v;
    }
    paramsChanged = true;
    return kResultOk;
}

tresult PLUGIN_API Processor::getState(IBStream* state)
{
    IBStreamer s(state, kLittleEndian);
    for (int i = 0; i < kParamCount; ++i)
        if (!s.writeFloat(paramValues[i])) return kResultFalse;
    return kResultOk;
}

void Processor::sendNoteOn(Vst::ProcessData& data, int pitch, int sampleOffset, float vel)
{
    Vst::Event on{};
    on.type = Vst::Event::kNoteOnEvent;
    on.noteOn.channel = 0;
    on.noteOn.pitch = pitch;
    on.noteOn.velocity = vel;
    on.sampleOffset = sampleOffset;
    data.outputEvents->addEvent(on);
}

void Processor::sendNoteOff(Vst::ProcessData& data, int pitch, int sampleOffset)
{
    Vst::Event off{};
    off.type = Vst::Event::kNoteOffEvent;
    off.noteOff.channel = 0;
    off.noteOff.pitch = pitch;
    off.noteOff.velocity = 0.f;
    off.sampleOffset = sampleOffset;
    data.outputEvents->addEvent(off);
}

void Processor::sendAllNotesOff(Vst::ProcessData& data, int sampleOffset)
{
    for (size_t n = 0; n < heldChord.size(); ++n)
    {
        if (noteOnFlags[n])
        {
            sendNoteOff(data, heldChord[n], sampleOffset);
            noteOnFlags[n] = false;
        }
    }
}

float Processor::computeVelocity() const
{
    float vel = std::pow(velocity, 1.5f);
    return 0.1f + vel * 0.9f;
}

int Processor::getStepSamples(int noteIndex, int barSamples) const
{
    int patternIndex = noteIndex % static_cast<int>(rhythmPatterns.size());
    int steps = std::max(1, rhythmPatterns[patternIndex]);
    return barSamples / steps;
}

int Processor::getGateSamples(int stepSamples) const
{
    return std::max(1, static_cast<int>(stepSamples * noteLength));
}

double Processor::applySwing(double posInStep, int64 stepIndex, double stepSampD) const
{
    if (swing <= 0.0f || (stepIndex % 2 == 0)) return posInStep;
    double offset = static_cast<double>(swing) * stepSampD * 0.5;
    return posInStep - offset;
}

int32 Processor::denormalizeRhythm(Vst::ParamValue normalized, int32 min, int32 max)
{
    return min + static_cast<int32>(normalized * (max - min) + 0.5);
}

void Processor::updateRhythmPatterns()
{
    // rhythm patterns (R1–R4)
    for (int i = 0; i < 4; ++i) {
        int v = denormalizeRhythm(paramValues[i], 1, 16);
        rhythmPatterns[i] = std::max(1, std::min(16, v));
    }

    // global continuous parameters
    velocity    = denorm(kParamVelocity);
    noteLength  = denorm(kParamNoteLength);
    swing       = denorm(kParamSwing);

    float barSize = denorm(kParamBarSize);   /////// 0 ... 4
    barMultiplier = barSize;                 /////// used directly as bar-length multiplier

    paramsChanged = false;
}

void Processor::reset()
{
    sampleCounter = 0;
    wasPlaying = false;
    noteOnFlags.assign(noteOnFlags.size(), false);
    rhythmCounters.assign(rhythmCounters.size(), 0);
}

/// midi input.
void Processor::processMIDIInput(Vst::ProcessData& data)
{
    if (!data.inputEvents) return;

    std::vector<int> newChord = inputChord;
    bool chordChanged = false;

    int32 numEvents = data.inputEvents->getEventCount();
    for (int32 i = 0; i < numEvents; ++i)
    {
        Vst::Event e;
        if (data.inputEvents->getEvent(i, e) != kResultOk) continue;

        if (e.type == Vst::Event::kNoteOnEvent && e.noteOn.velocity > 0)
        {
            if (std::find(newChord.begin(), newChord.end(), e.noteOn.pitch) == newChord.end())
                newChord.push_back(e.noteOn.pitch);
        }
        else if (e.type == Vst::Event::kNoteOffEvent ||
                 (e.type == Vst::Event::kNoteOnEvent && e.noteOn.velocity == 0))
        {
            int pitch = (e.type == Vst::Event::kNoteOffEvent) ? e.noteOff.pitch : e.noteOn.pitch;
            auto it = std::find(newChord.begin(), newChord.end(), pitch);
            if (it != newChord.end()) newChord.erase(it);
        }
    }

    std::sort(newChord.begin(), newChord.end());

    if (newChord != heldChord) chordChanged = true;

    if (newChord.empty())
    {
        sendAllNotesOff(data);
        heldChord.clear();
        inputChord.clear();
        reset();
        return;
    }

    if (chordChanged)
    {
        sendAllNotesOff(data);
        reset();

        heldChord = newChord;
        if (static_cast<int>(heldChord.size()) > maxNotes)
            heldChord.resize(maxNotes);

        rhythmCounters.resize(heldChord.size(), 0);
        noteOnFlags.resize(heldChord.size(), false);
        inputChord = heldChord;
    }
}

tresult PLUGIN_API Processor::process(Vst::ProcessData& data)
{
    if (data.numSamples == 0 || !data.outputEvents || !data.processContext)
        return kResultOk;

    if (data.inputParameterChanges) {
        int32 numParams = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < numParams; ++i)
        {
            auto* queue = data.inputParameterChanges->getParameterData(i);
            if (!queue) continue;
            Vst::ParamID id = queue->getParameterId();
            if (id >= kParamCount) continue;
            int32 np = queue->getPointCount();
            if (np <= 0) continue;
            Vst::ParamValue val; int32 offset;
            if (queue->getPoint(np - 1, offset, val) == kResultOk)
            {
                paramValues[id] = static_cast<float>(val);
                paramsChanged = true;
            }
        }
    }

    if (paramsChanged.exchange(false)) updateRhythmPatterns();

    processMIDIInput(data);

    bool isPlaying = data.processContext->state & Vst::ProcessContext::kPlaying;
    if (!isPlaying) { if (wasPlaying) sendAllNotesOff(data); reset(); return kResultOk; }
    if (heldChord.empty()) return kResultOk;

    double bpm = data.processContext->tempo > 0 ? data.processContext->tempo : 120.0;
    double beatsPerBar = data.processContext->timeSigNumerator;
    double barSamplesD = (beatsPerBar * 60.0 / bpm) * processSetup.sampleRate * barMultiplier;

    int64 blockStartSample = data.processContext->projectTimeSamples;
    m_firedThisBlock = {};

    for (int32 i = 0; i < data.numSamples; ++i)
    {
        int64 absSample = blockStartSample + i;
        double posInBarD = std::fmod(static_cast<double>(absSample), barSamplesD);

        for (size_t n = 0; n < heldChord.size(); ++n)
        {
            int steps = std::max(1, rhythmPatterns[n % rhythmPatterns.size()]);
            double stepSampD = barSamplesD / steps;
            double gateSampD = std::max(1.0, stepSampD * noteLength);

            int64 stepIndex = static_cast<int64>(posInBarD / stepSampD);
            double rawPosInStep = posInBarD - static_cast<double>(stepIndex) * stepSampD;
            double effectivePosInStep = applySwing(rawPosInStep, stepIndex, stepSampD);

            bool atStepStart = (effectivePosInStep >= 0.0 && effectivePosInStep < 1.0);
            bool gateOpen = (effectivePosInStep >= 0.0 && effectivePosInStep < gateSampD);

            if (atStepStart)
            {
                if (noteOnFlags[n]) { sendNoteOff(data, heldChord[n], i); noteOnFlags[n] = false; }
                sendNoteOn(data, heldChord[n], i, computeVelocity());
                noteOnFlags[n] = true;
                if (n < 4) m_firedThisBlock[n] = true;
            }
            else if (!gateOpen && noteOnFlags[n])
            {
                sendNoteOff(data, heldChord[n], i);
                noteOnFlags[n] = false;
            }
        }
        sampleCounter++;
    }


    if (data.outputParameterChanges)
    {
        static const Vst::ParamID triggerIDs[4] = {
            kParamTriggerA, kParamTriggerB, kParamTriggerC, kParamTriggerD
        };
        for (int n = 0; n < 4; ++n)
        {
            if (m_firedThisBlock[n])
            {
                int32 idx;
                auto* queue = data.outputParameterChanges->addParameterData(triggerIDs[n], idx);
                if (queue)
                    queue->addPoint(0, 1.0, idx);
            }
        }
    }

    wasPlaying = true;
    return kResultOk;
}

} // namespace oscilleon
} // namespace Steinberg
