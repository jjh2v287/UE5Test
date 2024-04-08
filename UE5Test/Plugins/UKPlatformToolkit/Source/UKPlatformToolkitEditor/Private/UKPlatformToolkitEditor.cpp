// Copyright Epic Games, Inc. All Rights Reserved.

#include "UKPlatformToolkitEditor.h"
#include "LevelEditor.h"
#include "Selection.h"
#include "UKPlatformToolkitEditorStyle.h"
#include "UKPlatformToolkitEditorCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "UKPlatformActor.h"
#include "UKPlatformGroupActor.h"
#include "UKPlatformToolUtils.h"
#include "Engine/Blueprint.h"

static const FName UKPlatformToolkitEditorTabName("UKPlatformToolkitEditor");

#define LOCTEXT_NAMESPACE "FUKPlatformToolkitEditorModule"

void FUKPlatformToolkitEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FUKPlatformToolkitEditorStyle::Initialize();
	FUKPlatformToolkitEditorStyle::ReloadTextures();

	FUKPlatformToolkitEditorCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FUKPlatformToolkitEditorCommands::Get().PlatformGroupBoxAction,
		// FExecuteAction::CreateStatic( &FUKPlatformToolModule::BrowseDocumentation),
		FExecuteAction::CreateRaw(this, &FUKPlatformToolkitEditorModule::PlatformGroup, true),
		FCanExecuteAction());

	PluginCommands->MapAction(
		FUKPlatformToolkitEditorCommands::Get().PlatformGroupSphereAction,
		// FExecuteAction::CreateStatic( &FUKPlatformToolModule::BrowseDocumentation),
		FExecuteAction::CreateRaw(this, &FUKPlatformToolkitEditorModule::PlatformGroup, false),
		FCanExecuteAction());

	PluginCommands->MapAction(
		FUKPlatformToolkitEditorCommands::Get().PlatformUnGroupAction,
		// FExecuteAction::CreateStatic( &FUKPlatformToolModule::BrowseDocumentation),
		FExecuteAction::CreateRaw(this, &FUKPlatformToolkitEditorModule::PlatformUnGroup),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	LevelEditorModule.GetGlobalLevelEditorActions()->Append(PluginCommands.ToSharedRef());

	FEditorDelegates::OnDeleteActorsBegin.AddRaw(this, &FUKPlatformToolkitEditorModule::OnDeleteActorsBegin);
	FEditorDelegates::OnDeleteActorsEnd.AddRaw(this, &FUKPlatformToolkitEditorModule::OnDeleteActorsEnd);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FUKPlatformToolkitEditorModule::RegisterMenus));
}

void FUKPlatformToolkitEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FUKPlatformToolkitEditorStyle::Shutdown();

	FUKPlatformToolkitEditorCommands::Unregister();
}

void FUKPlatformToolkitEditorModule::OnDeleteActorsBegin()
{
	TArray<AUKPlatformActor*> Actors;
	GEditor->GetSelectedActors()->GetSelectedObjects<AUKPlatformActor>(Actors);
	for (AUKPlatformActor* PlatformActor : Actors)
	{
		AUKPlatformGroupActor* PlatformGroupActor = PlatformActor->PlatformGroupActor;
		PlatformGroupActor->PlatformActors.Remove(PlatformActor);
		PlatformGroupActor->Modify();
	}
}

void FUKPlatformToolkitEditorModule::OnDeleteActorsEnd()
{
}

void FUKPlatformToolkitEditorModule::PlatformGroup(bool IsBox)
{
	UUKPlatformToolUtils::Get()->PlatformGroup(IsBox);
}

void FUKPlatformToolkitEditorModule::PlatformUnGroup()
{
	UUKPlatformToolUtils::Get()->PlatformUnGroup();
}

void FUKPlatformToolkitEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenus* ToolMenus = UToolMenus::Get();
		if (ToolMenus->IsMenuRegistered("LevelEditor.ActorContextMenu"))
		{
			return;
		}

		if (UUKPlatformToolUtils::Get() == nullptr)
		{
			UClass* ActorGroupingUtilsClass = UUKPlatformToolUtils::StaticClass();
			UUKPlatformToolUtils* ActorGroupingUtils = NewObject<UUKPlatformToolUtils>(GEditor, ActorGroupingUtilsClass);
		}

		UToolMenu* Menu = ToolMenus->ExtendMenu("LevelEditor.ActorContextMenu");
		Menu->AddDynamicSection("UKPlatformTool", FNewToolMenuDelegate::CreateLambda([](UToolMenu* InMenu)
		{
			if (GEditor && GEditor->GetSelectedComponentCount() == 0 && GEditor->GetSelectedActorCount() > 0)
			{
				FToolMenuSection& Section = InMenu->AddSection("UKPlatformTool", LOCTEXT("UKPlatformTool", "UKPlatformTool Group"));
				FUKSelectActorPlatformInfo PlatformInfo = UUKPlatformToolUtils::Get()->GetSelectPlatformGroupInfo();
				if(PlatformInfo.PlatformActorGroupNum == 0 && PlatformInfo.PlatformActorNum > 0)
				{
					Section.AddMenuEntry( FUKPlatformToolkitEditorCommands::Get().PlatformGroupSphereAction );
					Section.AddMenuEntry( FUKPlatformToolkitEditorCommands::Get().PlatformGroupBoxAction );
				}
				else if(PlatformInfo.PlatformActorGroupNum > 0)
				{
					Section.AddMenuEntry( FUKPlatformToolkitEditorCommands::Get().PlatformUnGroupAction );
				}
			}
		}));
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUKPlatformToolkitEditorModule, UKPlatformToolkitEditor)