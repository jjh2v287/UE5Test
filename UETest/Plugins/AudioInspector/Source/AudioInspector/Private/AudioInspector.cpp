// Copyright SweejTech Ltd. All Rights Reserved.

#include "AudioInspector.h"
#include "AudioInspectorStyle.h"
#include "AudioInspectorCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "SSweejActiveSoundsWidget.h"
#include "AudioInspectorEditorWindow.h"

#include "Modules/ModuleManager.h"
#include "Interfaces/IMainFrameModule.h"

static const FName AudioInspectorTabName("Audio Inspector");

#define LOCTEXT_NAMESPACE "FAudioInspectorModule"

FAudioInspectorModule& FAudioInspectorModule::GetModule() 
{ 
	return FModuleManager::GetModuleChecked<FAudioInspectorModule>("AudioInspector"); 
}

void FAudioInspectorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FAudioInspectorStyle::Initialize();
	FAudioInspectorStyle::ReloadTextures();

	FAudioInspectorCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FAudioInspectorCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FAudioInspectorModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAudioInspectorModule::RegisterMenus));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AudioInspectorTabName, FOnSpawnTab::CreateRaw(this, &FAudioInspectorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FAudioInspectorTabTitle", "Audio Inspector"))
		.SetTooltipText(LOCTEXT("AudioInspectorTabTooltipText", "SweejTech Audio Inspector"))
		.SetMenuType(ETabSpawnerMenuType::Hidden)
		.SetIcon(FSlateIcon(FAudioInspectorStyle::GetStyleSetName(), "AudioInspector.WindowIcon"));

	// Add this command list to the main frame so we can use key commands to create the tab
	IMainFrameModule& MainFrame = FModuleManager::Get().LoadModuleChecked<IMainFrameModule>("MainFrame");
	MainFrame.GetMainFrameCommandBindings()->Append(PluginCommands.ToSharedRef());

	MainFrame.OnMainFrameCreationFinished().AddRaw(this, &FAudioInspectorModule::OnMainFrameCreationFinished);
	
	// Ensure this module works with live coding - respond to a reload complete event
	FCoreUObjectDelegates::ReloadCompleteDelegate.AddRaw(this, &FAudioInspectorModule::OnReloadComplete);
}

void FAudioInspectorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FCoreUObjectDelegates::ReloadCompleteDelegate.RemoveAll(this);

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FAudioInspectorStyle::Shutdown();

	FAudioInspectorCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AudioInspectorTabName);
}

TSharedRef<SDockTab> FAudioInspectorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SAssignNew(AudioInspectorDockTab, SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SAssignNew(AudioInspectorEditorWindow, SAudioInspectorEditorWindow)
		]
		.OnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda([this](TSharedRef<SDockTab> DockTab) {
			// Reset the references when the tab closes otherwise it doesn't get shut down properly and can't be reopened
			ClearSlateReferences();
		}));
}

void FAudioInspectorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(AudioInspectorTabName);
}

void FAudioInspectorModule::SetUpdateInterval(float UpdateInterval)
{
	if (AudioInspectorEditorWindow.IsValid())
	{
		AudioInspectorEditorWindow->SetUpdateInterval(UpdateInterval);
	}
}

void FAudioInspectorModule::RebuildActiveSoundsColumns()
{
	if (AudioInspectorEditorWindow.IsValid())
	{
		AudioInspectorEditorWindow->RebuildActiveSoundsColumns();
	}
}

void FAudioInspectorModule::OnMainFrameCreationFinished(TSharedPtr<SWindow> Window, bool bShouldShowProjectDialogAtStartup)
{
	const IMainFrameModule& MainFrame = FModuleManager::Get().LoadModuleChecked<IMainFrameModule>("MainFrame");
	
	if (MainFrame.IsRecreatingDefaultMainFrame())
	{
		ClearSlateReferences();
	}
}

void FAudioInspectorModule::OnReloadComplete(EReloadCompleteReason Reason)
{
	if (AudioInspectorDockTab.IsValid())
	{
		AudioInspectorDockTab->SetContent(SNullWidget::NullWidget);
		AudioInspectorDockTab->SetContent(SAssignNew(AudioInspectorEditorWindow, SAudioInspectorEditorWindow));
	}
}

void FAudioInspectorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->AddSection("SweejTech", FText::FromString("SweejTech"));
			Section.AddMenuEntryWithCommandList(FAudioInspectorCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("SweejTech");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FAudioInspectorCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

void FAudioInspectorModule::ClearSlateReferences()
{
	AudioInspectorDockTab.Reset();
	AudioInspectorEditorWindow.Reset();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAudioInspectorModule, AudioInspector)