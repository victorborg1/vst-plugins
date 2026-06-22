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
    kParamDry = 0,
    kParamBuffer = 1,
    kParamGranulator = 2,
    kParamMaxGrains = 3,
    kParamInput = 4,
    kParamFeedback = 5,
    kParamBufferSize = 6,
    kParamFreeze = 7,
    kParamPosition = 8,
    kParamIncrement = 9,
    kParamRate = 10,
    kParamLength = 11,
    kParamPitch = 12,
    kParamReverse = 13,
    kParamPan = 14,
    kParamVolume = 15,
    kParamCount = 16
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
        { kParamDry,        STR16("DRY"),         STR16(""),    0,  1.0,                              0.0,   1.0  },
        { kParamBuffer,     STR16("BUFFER"),      STR16(""),    0,  0.0,                              0.0,   1.0  },
        { kParamGranulator, STR16("GRANULATOR"),  STR16(""),    0,  0.8,                              0.0,   1.0  },
        { kParamMaxGrains,  STR16("MAX GRAINS"),  STR16(""),   31,  toNormalized(16.0, 1.0, 32.0),    1.0,  32.0  },
        { kParamInput,      STR16("INPUT"),       STR16(""),    0,  1.0,                              0.0,   1.0  },
        { kParamFeedback,   STR16("FEEDBACK"),    STR16(""),    0,  0.0,                              0.0,   1.0  },
        { kParamBufferSize, STR16("BUFFER SIZE"), STR16("s"),   0,  toNormalized(3.0, 0.5, 6.0),      0.5,   6.0  },
        { kParamFreeze,     STR16("FREEZE"),      STR16(""),    1,  0.0,                              0.0,   1.0  },
        { kParamPosition,   STR16("POSITION"),    STR16(""),    0,  0.0,                              0.0,   1.0  },
        { kParamIncrement,  STR16("INCREMENT"),   STR16(""),    0,  0.5,                             -2.0,   2.0  },
        { kParamRate,       STR16("RATE"),        STR16(""),    0,  toNormalized(8.0, 1.0, 64.0),     1.0,  64.0  },
        { kParamLength,     STR16("LENGTH"),      STR16("ms"),  0,  toNormalized(100.0, 10.0, 500.0), 10.0, 500.0 },
        { kParamPitch,      STR16("PITCH"),       STR16("st"), 48,  toNormalized(0.0, -24.0, 24.0),  -24.0,  24.0 },
        { kParamReverse,    STR16("REVERSE"),     STR16(""),    0,  0.0,                              0.0,   1.0  },
        { kParamPan,        STR16("PAN"),         STR16(""),    0,  0.5,                             -1.0,   1.0  },
        { kParamVolume,     STR16("VOLUME"),      STR16(""),    0,  1.0,                              0.0,   1.0  },
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
            if (!s.readFloat(v)) return kResultFalse; else value[i] = static_cast<Vst::ParamValue>(v);
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
            auto* p = new Vst::RangeParameter(
                info.name, info.id, info.unit,
                info.min, info.max,
                fromNormalized(info.defaultValueNormalized, info.min, info.max),
                info.stepCount,
                Vst::ParameterInfo::kCanAutomate,
                Vst::kRootUnitId);
            if (p) parameters.addParameter(p);
        }
        return kResultOk;
    }

    std::array<Vst::ParamValue, kParamCount> value;
};

} // namespace oscilleon
} // namespace Steinberg