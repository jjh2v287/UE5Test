// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitTargetComponent.h"
#include "TickOptToolkitComponentCustomization.h"
#include "TickOptToolkitComponentVisualizer.h"

#include "TickOptToolkitAnimUpdateRateOptComponent.h"
#include "TickOptToolkitAnimUpdateRateOptimizationComponentCustomization.h"
#include "TickOptToolkitOptimizationLevelCustomization.h"
#include "TickOptToolkitUROOptimizationLevelCustomization.h"

#include "Modules/ModuleManager.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"

static const FName TickOptToolkitComponentClassName = UTickOptToolkitTargetComponent::StaticClass()->GetFName();
static const FName TickOptToolkitAnimUpdateRateOptComponentClassName = UTickOptToolkitAnimUpdateRateOptComponent::StaticClass()->GetFName();
static const FName TickOptToolkitOptimizationLevelStructName = FTickOptToolkitOptimizationLevel::StaticStruct()->GetFName();
static const FName TickOptToolkitUROOptimizationLevelStructName = FTickOptToolkitUROOptimizationLevel::StaticStruct()->GetFName();
static const FName PropertyEditorModuleName = "PropertyEditor";

class FTickOptToolkitEditor : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>(PropertyEditorModuleName);
		PropertyEditorModule.RegisterCustomClassLayout(TickOptToolkitComponentClassName, FOnGetDetailCustomizationInstance::CreateStatic(&FTickOptToolkitComponentCustomization::MakeInstance));
		PropertyEditorModule.RegisterCustomClassLayout(TickOptToolkitAnimUpdateRateOptComponentClassName, FOnGetDetailCustomizationInstance::CreateStatic(&FTickOptToolkitAnimUpdateRateOptComponentCustomization::MakeInstance));
		PropertyEditorModule.RegisterCustomPropertyTypeLayout(TickOptToolkitOptimizationLevelStructName, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTickOptToolkitOptimizationLevelCustomization::MakeInstance));
		PropertyEditorModule.RegisterCustomPropertyTypeLayout(TickOptToolkitUROOptimizationLevelStructName, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTickOptToolkitUROOptimizationLevelCustomization::MakeInstance));
	
		if (GUnrealEd)
		{
			GUnrealEd->RegisterComponentVisualizer(TickOptToolkitComponentClassName, MakeShareable(new FTickOptToolkitComponentVisualizer()));
		}
	}
	
	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded(PropertyEditorModuleName))
		{
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>(PropertyEditorModuleName);
			PropertyEditorModule.UnregisterCustomClassLayout(TickOptToolkitComponentClassName);
			PropertyEditorModule.UnregisterCustomClassLayout(TickOptToolkitAnimUpdateRateOptComponentClassName);
			PropertyEditorModule.UnregisterCustomPropertyTypeLayout(TickOptToolkitOptimizationLevelStructName);
			PropertyEditorModule.UnregisterCustomPropertyTypeLayout(TickOptToolkitUROOptimizationLevelStructName);
		}
		
		if (GUnrealEd)
		{
			GUnrealEd->UnregisterComponentVisualizer(TickOptToolkitComponentClassName);
		}
	}
};

IMPLEMENT_MODULE(FTickOptToolkitEditor, TickOptToolkitEditor);
