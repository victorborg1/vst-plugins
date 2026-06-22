

#include "Processor.h"
#include "Controller.h"
#include "cids.h"
#include "version.h"
#include "gui/Editor.h"
#include "gui/Entry.h"         
#include "Parameter.h"

#include "public.sdk/source/main/pluginfactory.h"

#define stringPluginName "Arp"

using namespace Steinberg::Vst;
using namespace Steinberg::oscilleon;

#define VST3Category PlugType::kFx

using PlugGuiEntry = gui::Entry<::oscilleon::gui::Editor, 800, 600>;
using PlugController = Controller<PlugGuiEntry, Parameters>;

BEGIN_FACTORY_DEF ( "EigenDSP", 
			       "https://www.eigendsp.com", 
			       "mailto:info@eigendsp.com")

	DEF_CLASS2 (INLINE_UID_FROM_FUID(kProcessorUID),
				PClassInfo::kManyInstances,		// cardinality
				kVstAudioEffectClass,			// the component category (do not changed this)
				stringPluginName,				// here the Plug-in name (to be changed)
				Vst::kDistributable,			// means that component and controller could be distributed on different computers
				VST3Category, 					// Subcategory for this Plug-in (to be changed)
				FULL_VERSION_STR,				// Plug-in version (to be changed)
				kVstVersionString,				// the VST 3 SDK version (do not changed this, use always this define)
				Processor::createInstance)		// function pointer called when this component should be instantiated

	// its kVstComponentControllerClass component
	DEF_CLASS2 (INLINE_UID_FROM_FUID (kControllerUID),
				PClassInfo::kManyInstances, 	// cardinality
				kVstComponentControllerClass,	// the Controller category (do not changed this)
				stringPluginName " Controller",	// controller name (could be the same than component name)
				0,								// not used here
				"",								// not used here
				FULL_VERSION_STR,				// Plug-in version (to be changed)
				kVstVersionString,				// the VST 3 SDK version (do not changed this, use always this define)
				PlugController::createInstance)	// function pointer called when this component should be instantiated


END_FACTORY
