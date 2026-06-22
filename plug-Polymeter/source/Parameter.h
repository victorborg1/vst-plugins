#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include <array>

namespace Steinberg {
namespace oscilleon {

inline std::string Utf16ToUtf8(const Steinberg::char16* str)
{
    std::string result;
    while (*str) {
        char16_t c = static_cast<char16_t>(*str++);
        if (c < 0x80)        result += static_cast<char>(c);
        else if (c < 0x800) { result += static_cast<char>(0xC0 | (c >> 6));  result += static_cast<char>(0x80 | (c & 0x3F)); }
        else { result += static_cast<char>(0xE0 | (c >> 12)); result += static_cast<char>(0x80 | ((c >> 6) & 0x3F)); result += static_cast<char>(0x80 | (c & 0x3F)); }
    }
    return result;
}

enum ParamID : Vst::ParamID
{
    kParamRhythmA = 0,
    kParamRhythmB,
    kParamRhythmC,
    kParamRhythmD,

    kParamNoteLength,
    kParamVelocity,
    kParamBarSize,
    kParamSwing,

    kParamTriggerA,
    kParamTriggerB,
    kParamTriggerC,
    kParamTriggerD,
    kParamTriggerSync,

    kParamCount
};

struct ParamInfo {
    ParamID                  id;
    const Steinberg::char16* name;
    const Steinberg::char16* unit;
    int32                    stepCount;
    Vst::ParamValue          defaultValueNormalized;
    Vst::ParamValue          min;
    Vst::ParamValue          max;
};

inline Vst::ParamValue toNormalized(Vst::ParamValue v, Vst::ParamValue mn, Vst::ParamValue mx) { return (v - mn) / (mx - mn); }
inline Vst::ParamValue fromNormalized(Vst::ParamValue n, Vst::ParamValue mn, Vst::ParamValue mx) { return mn + n * (mx - mn); }

inline const ParamInfo& getParamInfo(ParamID id)
{
    static const ParamInfo infos[] = {

        // Rhythm (1–16)
        { kParamRhythmA, STR16("R1"), STR16(""), 15, toNormalized(4.0, 1.0, 16.0), 1.0, 16.0 },
        { kParamRhythmB, STR16("R2"), STR16(""), 15, toNormalized(4.0, 1.0, 16.0), 1.0, 16.0 },
        { kParamRhythmC, STR16("R3"), STR16(""), 15, toNormalized(4.0, 1.0, 16.0), 1.0, 16.0 },
        { kParamRhythmD, STR16("R4"), STR16(""), 15, toNormalized(4.0, 1.0, 16.0), 1.0, 16.0 },

        // Continuous
        { kParamNoteLength, STR16("GATE"), STR16("%"), 0, toNormalized(0.5, 0.1, 1.0), 0.1, 1.0 },
        { kParamVelocity,   STR16("VEL"),  STR16(""),  0, 0.8,                          0.0, 1.0 },

        // Bar size (discrete 0–4)
        { kParamBarSize, STR16("BAR"), STR16(""), 4, toNormalized(2.0, 0.0, 4.0), 0.0, 4.0 },

        { kParamSwing, STR16("SWING"), STR16(""), 0, 0.0, 0.0, 1.0 },

        // Triggers (binary)
        { kParamTriggerA, STR16("Trigger A"), STR16(""), 1, 0.0, 0.0, 1.0 },
        { kParamTriggerB, STR16("Trigger B"), STR16(""), 1, 0.0, 0.0, 1.0 },
        { kParamTriggerC, STR16("Trigger C"), STR16(""), 1, 0.0, 0.0, 1.0 },
        { kParamTriggerD, STR16("Trigger D"), STR16(""), 1, 0.0, 0.0, 1.0 },
        { kParamTriggerSync, STR16("Trigger Sync"), STR16(""), 1, 0.0, 0.0, 1.0 },
    };

    return infos[id];
}

class Parameters {
public:
    Parameters() {
        for (int i = 0; i < kParamCount; ++i)
            value[i] = getParamInfo(static_cast<ParamID>(i)).defaultValueNormalized;
    }

    tresult setState(IBStream* state) {
        IBStreamer s(state, kLittleEndian);
        float v = 0.f;
        for (int i = 0; i < kParamCount; ++i)
            if (!s.readFloat(v)) return kResultFalse;
            else value[i] = static_cast<Vst::ParamValue>(v);
        return kResultOk;
    }

    tresult getState(IBStream* state) {
        IBStreamer s(state, kLittleEndian);
        for (int i = 0; i < kParamCount; ++i)
            if (!s.writeFloat(static_cast<float>(value[i]))) return kResultFalse;
        return kResultOk;
    }

    tresult addParameter(Vst::ParameterContainer& parameters) {
        for (int i = 0; i < kParamCount; ++i) {
            const auto& info = getParamInfo(static_cast<ParamID>(i));

            auto flags = (i >= kParamTriggerA)
                ? (Vst::ParameterInfo::kIsReadOnly | Vst::ParameterInfo::kIsHidden)
                : Vst::ParameterInfo::kCanAutomate;

            auto* p = new Vst::RangeParameter(
                info.name, info.id, info.unit,
                info.min, info.max,
                fromNormalized(info.defaultValueNormalized, info.min, info.max),
                info.stepCount,
                flags,
                Vst::kRootUnitId);

            if (p) parameters.addParameter(p);
        }
        return kResultOk;
    }

    std::array<Vst::ParamValue, kParamCount> value;
};

} // namespace oscilleon
} // namespace Steinberg