#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Steinberg {
namespace oscilleon {

template<typename EditorType, typename ParameterType>
class Controller
    : public Vst::EditControllerEx1
    , public Vst::IMidiMapping
{
public:
    Controller() = default;
    ~Controller() override {
        for (auto* ed : m_editors)
            if (ed) ed->forget();
    }

    static FUnknown* createInstance(void*) {
        return (Vst::IEditController*)new Controller<EditorType, ParameterType>();
    }

    tresult PLUGIN_API initialize(FUnknown* context) override {
        tresult result = EditControllerEx1::initialize(context);
        if (result != kResultOk) return result;
        ParameterType params;
        return params.addParameter(parameters);
    }

    tresult PLUGIN_API setComponentState(IBStream* state) override {
        if (!state) return kResultFalse;
        ParameterType params;
        if (params.setState(state) != kResultOk) return kResultFalse;
        for (size_t i = 0; i < params.value.size(); ++i)
            setParamNormalized(static_cast<Vst::ParamID>(i), params.value[i]);
        return kResultOk;
    }

    tresult PLUGIN_API setState(IBStream* state) override {
        ParameterType params;
        return params.setState(state);
    }

    tresult PLUGIN_API getState(IBStream* state) override {
        ParameterType params;
        for (size_t i = 0; i < params.value.size(); ++i)
            params.value[i] = getParamNormalized(static_cast<Vst::ParamID>(i));
        return params.getState(state);
    }

    IPlugView* PLUGIN_API createView(FIDString name) override {
        if (FIDStringsEqual(name, "editor")) {
            auto* ed = new EditorType(this);
            ed->remember();
            m_editors.push_back(ed);
            return ed;
        }
        return nullptr;
    }

    void editorDestroyed(Steinberg::IPlugView* view) {
        auto* ed = static_cast<EditorType*>(view);
        auto it = std::find(m_editors.begin(), m_editors.end(), ed);
        if (it != m_editors.end()) {
            (*it)->forget();
            m_editors.erase(it);
        }
    }

    tresult PLUGIN_API setParamNormalized(Vst::ParamID id, Vst::ParamValue value) override {
        value = std::clamp(value, 0.0, 1.0);
        tresult result = EditControllerEx1::setParamNormalized(id, value);
        if (result == kResultOk)
            for (auto* ed : m_editors)
                if (ed) ed->updateUI(id, value);
        return result;
    }

    tresult PLUGIN_API getMidiControllerAssignment(int32 busIndex, int16 channel,
        Vst::CtrlNumber midiControllerNumber, Vst::ParamID& id) override {
        if (busIndex == 0 && channel == 0) {
            if (midiControllerNumber >= 20 && midiControllerNumber <= 25) {
                id = midiControllerNumber - 20;
                return kResultOk;
            }
        }
        return kResultFalse;
    }

    tresult PLUGIN_API notify(Vst::IMessage* message) override {
        if (!message) return kResultFalse;
        if (strcmp(message->getMessageID(), "waveform") == 0) {
            auto* attr = message->getAttributes();

            const void* dataL = nullptr; uint32 sizeL = 0;
            const void* dataR = nullptr; uint32 sizeR = 0;
            int64  writePos = 0;
            double bufSize  = 0.0;

            attr->getBinary("L",        dataL,    sizeL);
            attr->getBinary("R",        dataR,    sizeR);
            attr->getFloat ("bufSize",  bufSize);
            attr->getInt   ("writePos", writePos);

            if (!dataL || !dataR) return kResultOk;

            const float* L = static_cast<const float*>(dataL);
            const float* R = static_cast<const float*>(dataR);
            uint32 count   = sizeL / sizeof(float);

            for (auto* ed : m_editors)
                if (ed)
                    ed->setWaveformData(L, R, count,
                                        static_cast<float>(bufSize),
                                        static_cast<uint32>(writePos));
            return kResultOk;
        }
        return EditControllerEx1::notify(message);
    }

    DEFINE_INTERFACES
        DEF_INTERFACE(Vst::IMidiMapping)
    END_DEFINE_INTERFACES(EditControllerEx1)
    DELEGATE_REFCOUNT(EditControllerEx1)

protected:
    std::vector<EditorType*> m_editors;
};

} // namespace oscilleon
} // namespace Steinberg