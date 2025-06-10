// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "SHTNBlackboardEditor.h"
#include "HTNAppStyle.h"
#include "HTNCommands.h"

#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "BehaviorTree/BlackboardData.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Modules/ModuleManager.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "ScopedTransaction.h"
#include "SGraphActionMenu.h"
#include "Runtime/Launch/Resources/Version.h"

class SWidget;

#define LOCTEXT_NAMESPACE "SHTNBlackboardEditor"

DEFINE_LOG_CATEGORY(LogBlackboardEditor);

void SHTNBlackboardEditor::Construct(const FArguments& InArgs, TSharedRef<FUICommandList> InCommandList, UBlackboardData* InBlackboardData)
{
	OnEntrySelected = InArgs._OnEntrySelected;
	OnGetDebugKeyValue = InArgs._OnGetDebugKeyValue;
	OnIsDebuggerReady = InArgs._OnIsDebuggerReady;
	OnIsDebuggerPaused = InArgs._OnIsDebuggerPaused;
	OnGetDebugTimeStamp = InArgs._OnGetDebugTimeStamp;
	OnGetDisplayCurrentState = InArgs._OnGetDisplayCurrentState;
	OnIsBlackboardModeActive = InArgs._OnIsBlackboardModeActive;

	const TSharedRef<FUICommandList> CommandList = MakeShareable(new FUICommandList);

	CommandList->MapAction(
		FHTNBlackboardCommands::Get().DeleteEntry,
		FExecuteAction::CreateSP(this, &SHTNBlackboardEditor::HandleDeleteEntry),
		FCanExecuteAction::CreateSP(this, &SHTNBlackboardEditor::CanDeleteEntry));

	CommandList->MapAction(
		FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &SHTNBlackboardEditor::HandleRenameEntry),
		FCanExecuteAction::CreateSP(this, &SHTNBlackboardEditor::CanRenameEntry));

	InCommandList->Append(CommandList);

	SHTNBlackboardView::Construct(
		SHTNBlackboardView::FArguments()
		.OnEntrySelected(InArgs._OnEntrySelected)
		.OnGetDebugKeyValue(InArgs._OnGetDebugKeyValue)
		.OnGetDisplayCurrentState(InArgs._OnGetDisplayCurrentState)
		.OnIsDebuggerReady(InArgs._OnIsDebuggerReady)
		.OnIsDebuggerPaused(InArgs._OnIsDebuggerPaused)
		.OnGetDebugTimeStamp(InArgs._OnGetDebugTimeStamp)
		.OnBlackboardKeyChanged(InArgs._OnBlackboardKeyChanged)
		.IsReadOnly(false),
		CommandList,
		InBlackboardData
	);
}

void SHTNBlackboardEditor::FillContextMenu(FMenuBuilder& MenuBuilder) const
{
	if (!IsDebuggerActive() && HasSelectedItems())
	{
		MenuBuilder.AddMenuEntry(FHTNBlackboardCommands::Get().DeleteEntry);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename, NAME_None, 
			LOCTEXT("Rename", "Rename"), 
			LOCTEXT("Rename_Tooltip", "Renames this blackboard entry."));
	}
}

void SHTNBlackboardEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder) const
{
	ToolbarBuilder.AddComboButton(
		FUIAction(
			FExecuteAction(),
			FCanExecuteAction::CreateSP(this, &SHTNBlackboardEditor::CanCreateNewEntry)), 
		FOnGetContent::CreateSP(this, &SHTNBlackboardEditor::HandleCreateNewEntryMenu),
		LOCTEXT("New_Label", "New Key" ),
		LOCTEXT("New_ToolTip", "Create a new blackboard entry" ),
		FSlateIcon(FHTNAppStyleGetAppStyleSetName(), "BTEditor.Blackboard.NewEntry")
	);
}

TSharedPtr<FExtender> SHTNBlackboardEditor::GetToolbarExtender(TSharedRef<FUICommandList> ToolkitCommands) const
{
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("Debugging", EExtensionHook::Before, ToolkitCommands, 
		FToolBarExtensionDelegate::CreateSP(this, &SHTNBlackboardEditor::FillToolbar));

	return ToolbarExtender;
}

bool SHTNBlackboardEditor::CanCreateNewEntry() const
{
	if (OnIsDebuggerReady.IsBound())
	{
		return !OnIsDebuggerReady.Execute();
	}

	return true;
}

bool SHTNBlackboardEditor::CanDeleteEntry() const
{
	const bool bModeActive = OnIsBlackboardModeActive.IsBound() && OnIsBlackboardModeActive.Execute();

	if (!IsDebuggerActive() && bModeActive)
	{
		bool bIsInherited = false;
		FBlackboardEntry* BlackboardEntry = GetSelectedEntry(bIsInherited);
		if (BlackboardEntry != nullptr)
		{
			return !bIsInherited;
		}
	}

	return false;
}

bool SHTNBlackboardEditor::CanRenameEntry() const
{
	const bool bModeActive = OnIsBlackboardModeActive.IsBound() && OnIsBlackboardModeActive.Execute();

	if (!IsDebuggerActive() && bModeActive)
	{
		bool bIsInherited = false;
		FBlackboardEntry* BlackboardEntry = GetSelectedEntry(bIsInherited);
		if (BlackboardEntry != nullptr)
		{
			return !bIsInherited;
		}
	}

	return false;
}

TSharedRef<SWidget> SHTNBlackboardEditor::HandleCreateNewEntryMenu() const
{
	class FBlackboardEntryClassFilter : public IClassViewerFilter
	{
	public:
		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
			const UClass* InClass,
			TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			return InClass && !InClass->HasAnyClassFlags(CLASS_Abstract | CLASS_HideDropDown) &&
				InClass->HasAnyClassFlags(CLASS_EditInlineNew) &&
				InClass->IsChildOf(UBlackboardKeyType::StaticClass());
			return false;
		}

		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
			const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
			TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			return InUnloadedClassData->IsChildOf(UBlackboardKeyType::StaticClass());
		}
	};

	FClassViewerInitializationOptions Options;
	Options.bShowUnloadedBlueprints = true;
	Options.NameTypeToDisplay = EClassViewerNameTypeToDisplay::DisplayName;
#if ENGINE_MAJOR_VERSION < 5
	Options.ClassFilter = MakeShared<FBlackboardEntryClassFilter>();
#else
	Options.ClassFilters.Add(MakeShared<FBlackboardEntryClassFilter>());
#endif

	// clear the search box, just in case there's something typed in there 
	// We need to do that since key adding code takes advantage of selection mechanics
	TSharedRef<SEditableTextBox> FilterTextBox = GraphActionMenu->GetFilterTextBox();
	FilterTextBox->SetText(FText());

	return
		SNew(SBox)
		.HeightOverride(240.0f)
		.WidthOverride(200.0f)
		[
			FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(Options,
				FOnClassPicked::CreateRaw(const_cast<SHTNBlackboardEditor*>(this),
					&SHTNBlackboardEditor::HandleCreateEntry))
		];
}

void SHTNBlackboardEditor::HandleCreateEntry(UClass* InClass)
{
	if (!BlackboardData)
	{
		UE_LOG(LogBlackboardEditor, Error,
			TEXT("Trying to delete an entry from a blackboard while no Blackboard Asset is set!"));
		return;
	}

	check(InClass);
	check(InClass->IsChildOf(UBlackboardKeyType::StaticClass()));

	FSlateApplication::Get().DismissAllMenus();

	const FScopedTransaction Transaction(LOCTEXT("BlackboardEntryAddTransaction", "Add Blackboard Entry"));
	BlackboardData->SetFlags(RF_Transactional);
	BlackboardData->Modify();

	FProperty* const KeysProperty = FindFProperty<FProperty>(UBlackboardData::StaticClass(),
		GET_MEMBER_NAME_CHECKED(UBlackboardData, Keys));
	BlackboardData->PreEditChange(KeysProperty);

	FBlackboardEntry Entry;
	Entry.EntryName = MakeUniqueKeyName(*InClass);
	Entry.KeyType = NewObject<UBlackboardKeyType>(BlackboardData, InClass);
	BlackboardData->Keys.Add(Entry);

	// Make the new entry selected in the list
	GraphActionMenu->RefreshAllActions(true);
	OnBlackboardKeyChanged.ExecuteIfBound(BlackboardData, &BlackboardData->Keys.Last());
	GraphActionMenu->SelectItemByName(Entry.EntryName, ESelectInfo::OnMouseClick);

	// Mark newly created entry as 'new'
	TArray<TSharedPtr<FEdGraphSchemaAction>> SelectedActions;
	GraphActionMenu->GetSelectedActions(SelectedActions);
	check(SelectedActions.Num() == 1);
	check(SelectedActions[0]->GetTypeId() == FEdGraphSchemaAction_BlackboardEntry::StaticGetTypeId());
	const TSharedPtr<FEdGraphSchemaAction_BlackboardEntry> BlackboardEntryAction = 
		StaticCastSharedPtr<FEdGraphSchemaAction_BlackboardEntry>(SelectedActions[0]);
	BlackboardEntryAction->bIsNew = true;

	GraphActionMenu->OnRequestRenameOnActionNode();

	FPropertyChangedEvent PropertyChangedEvent(KeysProperty, EPropertyChangeType::ArrayAdd);
	BlackboardData->PostEditChangeProperty(PropertyChangedEvent);
}

void SHTNBlackboardEditor::HandleDeleteEntry()
{
	if (BlackboardData == nullptr)
	{
		UE_LOG(LogBlackboardEditor, Error, TEXT("Trying to delete an entry from a blackboard while no Blackboard Asset is set!"));
		return;
	}

	if (!IsDebuggerActive())
	{
		bool bIsInherited = false;
		FBlackboardEntry* BlackboardEntry = GetSelectedEntry(bIsInherited);
		if (BlackboardEntry != nullptr && !bIsInherited)
		{
			const FScopedTransaction Transaction(LOCTEXT("BlackboardEntryDeleteTransaction", "Delete Blackboard Entry"));
			BlackboardData->SetFlags(RF_Transactional);
			BlackboardData->Modify();

			FProperty* KeysProperty = FindFProperty<FProperty>(UBlackboardData::StaticClass(), GET_MEMBER_NAME_CHECKED(UBlackboardData, Keys));
			BlackboardData->PreEditChange(KeysProperty);
		
			for (int32 ItemIndex = 0; ItemIndex < BlackboardData->Keys.Num(); ItemIndex++)
			{
				if (BlackboardEntry == &BlackboardData->Keys[ItemIndex])
				{
					BlackboardData->Keys.RemoveAt(ItemIndex);
					break;
				}
			}

			GraphActionMenu->RefreshAllActions(true);
			OnBlackboardKeyChanged.ExecuteIfBound(BlackboardData, nullptr);

			// signal de-selection
			if (OnEntrySelected.IsBound())
			{
				OnEntrySelected.Execute(nullptr, false);
			}

			FPropertyChangedEvent PropertyChangedEvent(KeysProperty, EPropertyChangeType::ArrayRemove);
			BlackboardData->PostEditChangeProperty(PropertyChangedEvent);
		}
	}
}

void SHTNBlackboardEditor::HandleRenameEntry()
{
	if (!IsDebuggerActive())
	{
		GraphActionMenu->OnRequestRenameOnActionNode();
	}
}

FName SHTNBlackboardEditor::MakeUniqueKeyName(const UClass& Class) const
{
	FString KeyName = Class.GetDisplayNameText().ToString();

	KeyName = KeyName.Replace(TEXT(" "), TEXT(""));
	KeyName += TEXT("Key");
	int32 IndexSuffix = INDEX_NONE;
	const auto HandlePotentialDuplicateName = [&](const FBlackboardEntry& Key)
	{
		if (Key.EntryName.ToString() == KeyName)
		{
			IndexSuffix = FMath::Max(0, IndexSuffix);
		}
		if (Key.EntryName.ToString().StartsWith(KeyName))
		{
			const FString ExistingSuffix = Key.EntryName.ToString().RightChop(KeyName.Len());
			if (ExistingSuffix.IsNumeric())
			{
				IndexSuffix = FMath::Max(FCString::Atoi(*ExistingSuffix) + 1, IndexSuffix);
			}
		}
	};

	for (const auto& Key : BlackboardData->Keys) { HandlePotentialDuplicateName(Key); };
	for (const auto& Key : BlackboardData->ParentKeys) { HandlePotentialDuplicateName(Key); };

	if (IndexSuffix != INDEX_NONE)
	{
		KeyName += FString::Printf(TEXT("%d"), IndexSuffix);
	}

	return FName(KeyName);
}

#undef LOCTEXT_NAMESPACE
