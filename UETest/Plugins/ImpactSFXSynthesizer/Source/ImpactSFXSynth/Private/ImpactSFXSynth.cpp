// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ImpactSFXSynth.h"
#include "MetasoundFrontendRegistries.h"
#include "MetasoundResidualData.h"
#include "MetasoundImpactModalObj.h"
#include "MetasoundMultiImpactData.h"
#include "ModalSpatialParameterInterface.h"
#include "ModalSpatialSourceDataOverride.h"

#define LOCTEXT_NAMESPACE "FImpactSFXSynthModule"

namespace Metasound
{
	REGISTER_METASOUND_DATATYPE(Metasound::FResidualData, "ResidualData", Metasound::ELiteralType::UObjectProxy, UResidualData);
	REGISTER_METASOUND_DATATYPE(Metasound::FImpactModalObj, "ModalObj", Metasound::ELiteralType::UObjectProxy, UImpactModalObj);
	REGISTER_METASOUND_DATATYPE(Metasound::FMultiImpactData, "MultiImpactData", Metasound::ELiteralType::UObjectProxy, UMultiImpactData);
}

TAudioSourceDataOverridePtr FSourceDataOverridePluginFactory::CreateNewSourceDataOverridePlugin(FAudioDevice* OwningDevice)
{
	return TAudioSourceDataOverridePtr(new FModalSpatialSourceDataOverride());
}

void FImpactSFXSynthModule::StartupModule()
{
	IModularFeatures::Get().RegisterModularFeature(FSourceDataOverridePluginFactory::GetModularFeatureName(), &SourceDataOverridePluginFactory);
	ModalSpatialParameterInterface::RegisterInterface();
	
	// All MetaSound interfaces are required to be loaded prior to registering & loading MetaSound assets,
	// so check that the MetaSoundEngine is loaded prior to pending Modulation defined classes
	FModuleManager::Get().LoadModuleChecked("MetasoundEngine");
	
	FMetasoundFrontendRegistryContainer::Get()->RegisterPendingNodes();
}

void FImpactSFXSynthModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

IAudioPluginFactory* FImpactSFXSynthModule::GetPluginFactory(EAudioPlugin PluginType)
{
	switch (PluginType)
	{
	case EAudioPlugin::SOURCEDATAOVERRIDE:
		return &SourceDataOverridePluginFactory;
		break;
	default:
		return nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FImpactSFXSynthModule, ImpactSFXSynth)