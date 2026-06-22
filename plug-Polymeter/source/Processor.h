#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "Parameter.h"
#include <atomic>
#include <array>
#include <vector>
#include <random>

namespace Steinberg {
namespace oscilleon {

class Processor : public Vst::AudioEffect
{
public:
    Processor();
    ~Processor() override;
    static FUnknown* createInstance(void*) { 
        return (Vst::IAudioProcessor*)new Processor; 
    }

    tresult PLUGIN_API initialize(FUnknown* context)                  override;
    tresult PLUGIN_API terminate()                                    override;
    tresult PLUGIN_API setActive(TBool state)                         override;
    tresult PLUGIN_API setupProcessing(Vst::ProcessSetup& newSetup)   override;
    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;
    tresult PLUGIN_API process(Vst::ProcessData& data)                override;
    tresult PLUGIN_API setState(IBStream* state)                      override;
    tresult PLUGIN_API getState(IBStream* state)                      override;

protected:
    /* MIDI stuff */
    void processMIDIInput(Vst::ProcessData& data);

    /* Rhythm stuff */
    void updateRhythmPatterns();
    int32 denormalizeRhythm(Vst::ParamValue normalized, int32 min, int32 max);
    int getStepSamples(int noteIndex, int barSamples) const;
    int getGateSamples(int stepSamples) const;
    double applySwing(double posInStep, int64 stepIndex, double stepSampD) const;
    float denorm(ParamID id) const;

    /* events */
    void sendNoteOn(Vst::ProcessData& data, int pitch, int sampleOffset, float vel);
    void sendNoteOff(Vst::ProcessData& data, int pitch, int sampleOffset);
    void sendAllNotesOff(Vst::ProcessData& data, int sampleOffset = 0);
    
    /* velocity */
    float computeVelocity() const;

    /* state */
    void reset();

    /* Rhythm state */
    std::array<int, 4>   rhythmPatterns = { 3, 4, 5, 7 };

    /* Chord state */
    std::vector<int>      heldChord;
    std::vector<int>      inputChord;

    /* Playback state */
    std::vector<Steinberg::int32> rhythmCounters;
    std::vector<bool>             noteOnFlags;
    Steinberg::int32              sampleCounter = 0;
    bool                          wasPlaying = false;

    /* params */
    float paramValues[kParamCount] = {};
    float velocity = 0.8f;
    float noteLength = 0.5f;
    int   maxNotes = 4;
    float barMultiplier = 1.0f;
    float swing = 0.0f;

    std::atomic<bool> paramsChanged{ false };

    /* trigger feedback */
    std::array<bool, 4> m_firedThisBlock = {};
};

} // namespace oscilleon
} // namespace Steinberg