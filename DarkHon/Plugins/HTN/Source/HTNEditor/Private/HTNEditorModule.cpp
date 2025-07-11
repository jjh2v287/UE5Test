// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNEditorModule.h"

#include "AssetTypeActions_HTN.h"
#include "DetailCustomizations/HTNBlackboardDecoratorDetails.h"
#include "DetailCustomizations/WorldstateSetValueContainerDetails.h"
#include "HTNEditor.h"
#include "HTNGraphNode.h"
#include "HTNNode.h"
#include "HTNRuntimeSettings.h"
#include "HTNStyle.h"
#include "SGraphNode_HTN.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "HTNEditorModule"

const FName HTNEditorAppIdentifier = FName(TEXT("HTNEditorApp"));

namespace
{
	class FGraphPanelNodeFactory_HTN : public FGraphPanelNodeFactory
	{
		virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override
		{
			if (UHTNGraphNode* const HTNNode = Cast<UHTNGraphNode>(Node))
			{
				return SNew(SGraphNode_HTN, HTNNode);
			}

			return nullptr;
		}
	};
}

// Editor module implementation
class FHTNEditorModule : public IHTNEditorModule
{
public:
	// IHasMenuExtensibility interface
	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() override { return MenuExtensibilityManager; }

	// IHasToolBarExtensibility interface
	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() override { return ToolBarExtensibilityManager; }

	// Begin IModuleInterface
	virtual void StartupModule() override
	{
		FHTNStyle::Initialize();

		MenuExtensibilityManager = MakeShared<FExtensibilityManager>();
		ToolBarExtensibilityManager = MakeShared<FExtensibilityManager>();

		FEdGraphUtilities::RegisterVisualNodeFactory(GraphNodeWidgetFactory = MakeShared<FGraphPanelNodeFactory_HTN>());

		// Register asset type actions
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		const auto RegisterAssetTypeAction = [&](TSharedRef<IAssetTypeActions> Action)
		{
			AssetTools.RegisterAssetTypeActions(Action);
			CreatedAssetTypeActions.Add(Action);
		};
		RegisterAssetTypeAction(MakeShared<FAssetTypeActions_HTN>());

		// Register the details customizer
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyModule.RegisterCustomPropertyTypeLayout(TEXT("WorldstateSetValueContainer"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWorldstateSetValueContainerDetails::MakeInstance));
		PropertyModule.RegisterCustomClassLayout(TEXT("HTNDecorator_Blackboard"), FOnGetDetailCustomizationInstance::CreateStatic(&FHTNBlackboardDecoratorDetails::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();

		// Register project settings
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule->RegisterSettings("Project", "Plugins", "HTN",
				LOCTEXT("HTNSettingsName", "HTN"),
				LOCTEXT("HTNSettingsDescription", "Configure the HTN plugin"),
				GetMutableDefault<UHTNRuntimeSettings>());
		}
	}

	virtual void ShutdownModule() override
	{
		MenuExtensibilityManager.Reset();
		ToolBarExtensibilityManager.Reset();

		if (GraphNodeWidgetFactory.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualNodeFactory(GraphNodeWidgetFactory);
			GraphNodeWidgetFactory.Reset();
		}	

		// Unregister asset type actions
		if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
			for (TSharedPtr<IAssetTypeActions>& Action : CreatedAssetTypeActions)
			{
				if (Action.IsValid())
				{
					AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
				}
			}
		}
		CreatedAssetTypeActions.Empty();

		// Unregister the details customizations
		if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
			PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("WorldstateSetValueContainer"));
			PropertyModule.UnregisterCustomClassLayout(TEXT("HTNDecorator_Blackboard"));
			PropertyModule.NotifyCustomizationModuleChanged();
		}

		// Unregister project settings
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule->UnregisterSettings("Project", "Plugins", "HTN");
		}

		FHTNStyle::Shutdown();
	}
	// End IModuleInterface

	
	// Begin IHTNEditorModule interface
	virtual TSharedRef<IHTNEditor> CreateHTNEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UHTN* HTN) override
	{
		if (!ClassCache.IsValid())
		{
			ClassCache = MakeShared<FGraphNodeClassHelper>(UHTNNode::StaticClass());
		}
		
		TSharedRef<FHTNEditor> NewHTNEditor = MakeShared<FHTNEditor>();
		NewHTNEditor->InitHTNEditor(Mode, InitToolkitHost, HTN);
		return NewHTNEditor;
	}

	virtual TSharedPtr<FGraphNodeClassHelper> GetClassCache() override { return ClassCache; }
	// End IHTNEditorModule interface

private:
	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;
	TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager;

	TArray<TSharedPtr<IAssetTypeActions>> CreatedAssetTypeActions;
	
	TSharedPtr<FGraphNodeClassHelper> ClassCache;

	static TSharedPtr<FGraphPanelNodeFactory_HTN> GraphNodeWidgetFactory;
};

TSharedPtr<FGraphPanelNodeFactory_HTN> FHTNEditorModule::GraphNodeWidgetFactory;

IMPLEMENT_GAME_MODULE(FHTNEditorModule, HTNEditor);

#undef LOCTEXT_NAMESPACE
