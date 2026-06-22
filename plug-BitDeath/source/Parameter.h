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

//used in editor to set the names of gui components which use utf8
//should probably fix this and remove this later
inline std::string Utf16ToUtf8(const Steinberg::char16* str)
{
    std::string result;
    while (*str) {
        char16_t c = static_cast<char16_t>(*str++);
        if (c < 0x80)        result += static_cast<char>(c);
        else if (c < 0x800) { result += char(0xC0|(c>>6)); result += char(0x80|(c&0x3F)); }
        else { result += char(0xE0|(c>>12)); result += char(0x80|((c>>6)&0x3F)); result += char(0x80|(c&0x3F)); }
    }
    return result;
}

enum ParamID : Vst::ParamID
{
    kParamDownsample = 0,
    kParamBitDepth,
    kParamMix,
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

inline Vst::ParamValue toNormalized (Vst::ParamValue v, Vst::ParamValue mn, Vst::ParamValue mx) { return (v-mn)/(mx-mn); }
inline Vst::ParamValue fromNormalized(Vst::ParamValue n, Vst::ParamValue mn, Vst::ParamValue mx) { return mn+n*(mx-mn); }

inline const ParamInfo& getParamInfo(ParamID id)
{
    static const ParamInfo infos[] = {
        { kParamDownsample, STR16("Sample Rate"), STR16("Hz"), 0,  1.0, 1000.0, 44100.0 },
        { kParamBitDepth,   STR16("Bits"),        STR16(""),  23, 1.0,    1.0,    24.0  },
        { kParamMix,        STR16("Mix"),         STR16(""),   0, 1.0,    0.0,     1.0  }
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
            if (!s.readFloat(v)) return kResultFalse; else value[i] = v;
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
                info.name, 
                info.id, 
                info.unit,
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