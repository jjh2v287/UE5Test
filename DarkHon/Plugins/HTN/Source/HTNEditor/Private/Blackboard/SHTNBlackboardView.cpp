// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "SHTNBlackboardView.h"

#include "SHTNBlackboardEditor.h"
#include "HTNAppStyle.h"
#include "HTNCommands.h"
#include "HTNTypes.h"

#include "Misc/EngineVersionComparison.h"
#if UE_VERSION_OLDER_THAN(5, 1, 0)
#include "ARFilter.h"
#include "AssetRegistryModule.h"
#else
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#include "Styling/SlateBrush.h"
#include "Fonts/SlateFontInfo.h"
#include "Misc/Paths.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardAssetProvider.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Commands/UICommandList.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "SGraphActionMenu.h"
#include "SGraphPalette.h"
#include "Styling/SlateIconFinder.h"
#include "Styling/CoreStyle.h"
#include "ScopedTransaction.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "SHTNBlackboardView"

#if UE_VERSION_OLDER_THAN(4, 25, 0)

namespace
{
	template <class T>
	typename TEnableIf<TIsDerivedFrom<T, FField>::IsDerived, T*>::Type FindFProperty(const UStruct* Owner, FName FieldName)
	{
		static_assert(sizeof(T) > 0, "T must not be an incomplete type");

		// We know that a "none" field won't exist in this Struct
		if (FieldName.IsNone())
		{
			return nullptr;
		}

		// Search by comparing FNames (INTs), not strings
		for (TFieldIterator<T>It(Owner); It; ++It)
		{
			if (It->GetFName() == FieldName)
			{
				return *It;
			}
		}

		// If we didn't find it, return no field
		return nullptr;
	}
}

#endif

namespace EBlackboardSectionTitles
{
	enum Type
	{
		InheritedKeys = 1,
		Keys,
	};
}

FName FEdGraphSchemaAction_BlackboardEntry::StaticGetTypeId() 
{ 
	static FName Type("FEdGraphSchemaAction_BlackboardEntry"); return Type; 
}

FName FEdGraphSchemaAction_BlackboardEntry::GetTypeId() const 
{ 
	return StaticGetTypeId(); 
}

FEdGraphSchemaAction_BlackboardEntry::FEdGraphSchemaAction_BlackboardEntry(UBlackboardData* InBlackboardData, 
	FBlackboardEntry& InKey, bool bInIsInherited)
	: FEdGraphSchemaAction_Dummy()
	, BlackboardData(InBlackboardData)
	, Key(InKey)
	, bIsInherited(bInIsInherited)
	, bIsNew(false)
{
	check(BlackboardData);
	Update();
}

void FEdGraphSchemaAction_BlackboardEntry::Update()
{
	UpdateSearchData(
		FText::FromName(Key.EntryName), 
		FText::Format(LOCTEXT("BlackboardEntryFormat", "{0} '{1}'"), 
			Key.KeyType ? Key.KeyType->GetClass()->GetDisplayNameText() : LOCTEXT("NullKeyDesc", "None"), 
			FText::FromName(Key.EntryName)
		),
		Key.EntryCategory.IsNone() ? FText() : FText::FromName(Key.EntryCategory), 
		FText());
	SectionID = bIsInherited ? EBlackboardSectionTitles::InheritedKeys : EBlackboardSectionTitles::Keys;
}

class SHTNBlackboardItem : public SGraphPaletteItem
{
	SLATE_BEGIN_ARGS( SHTNBlackboardItem ) {}

		SLATE_EVENT(FOnGetDebugKeyValue, OnGetDebugKeyValue)
		SLATE_EVENT(FOnGetDisplayCurrentState, OnGetDisplayCurrentState)
		SLATE_EVENT(FOnIsDebuggerReady, OnIsDebuggerReady)
		SLATE_EVENT(FOnBlackboardKeyChanged, OnBlackboardKeyChanged)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData)
	{
		OnGetDebugKeyValue = InArgs._OnGetDebugKeyValue;
		OnIsDebuggerReady = InArgs._OnIsDebuggerReady;
		OnGetDisplayCurrentState = InArgs._OnGetDisplayCurrentState;
		OnBlackboardKeyChanged = InArgs._OnBlackboardKeyChanged;

		const FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);

		check(InCreateData);
		check(InCreateData->Action.IsValid());

		const TSharedPtr<FEdGraphSchemaAction> GraphAction = InCreateData->Action;
		check(GraphAction->GetTypeId() == FEdGraphSchemaAction_BlackboardEntry::StaticGetTypeId());
		const TSharedPtr<FEdGraphSchemaAction_BlackboardEntry> BlackboardEntryAction = 
			StaticCastSharedPtr<FEdGraphSchemaAction_BlackboardEntry>(GraphAction);

		ActionPtr = InCreateData->Action;
		
		FSlateBrush const* IconBrush = FHTNAppStyle::GetBrush(TEXT("NoBrush"));
		GetPaletteItemIcon(GraphAction, IconBrush);

		TSharedRef<SWidget> IconWidget = CreateIconWidget(GraphAction->GetTooltipDescription(), IconBrush, FLinearColor::White);
		TSharedRef<SWidget> NameSlotWidget = CreateTextSlotWidget(
#if UE_VERSION_OLDER_THAN(5, 0, 0)
			NameFont,
#endif
			InCreateData, BlackboardEntryAction->bIsInherited );
		TSharedRef<SWidget> DebugSlotWidget = CreateDebugSlotWidget(NameFont);

		// Create the actual widget
		this->ChildSlot
		[
			SNew(SHorizontalBox)
			// Icon slot
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				IconWidget
			]
			// Name slot
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(3,0)
			[
				NameSlotWidget
			]
			// Debug info slot
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(3,0)
			[
				DebugSlotWidget
			]
		];
	}

private:
	void GetPaletteItemIcon(TSharedPtr<FEdGraphSchemaAction> InGraphAction, FSlateBrush const*& OutIconBrush)
	{
		check(InGraphAction.IsValid());
		check(InGraphAction->GetTypeId() == FEdGraphSchemaAction_BlackboardEntry::StaticGetTypeId());
		const TSharedPtr<FEdGraphSchemaAction_BlackboardEntry> BlackboardEntryAction = 
			StaticCastSharedPtr<FEdGraphSchemaAction_BlackboardEntry>(InGraphAction);

		if (ensure(BlackboardEntryAction) && BlackboardEntryAction->Key.KeyType)
		{
			OutIconBrush = FSlateIconFinder::FindIconBrushForClass(BlackboardEntryAction->Key.KeyType->GetClass());
		}
	}

	virtual TSharedRef<SWidget> CreateTextSlotWidget(
#if UE_VERSION_OLDER_THAN(5, 0, 0)
		const FSlateFontInfo& NameFont,
#endif
		FCreateWidgetForActionData* const InCreateData, 
		TAttribute<bool> bInIsReadOnly) override
	{
		check(InCreateData);

		TSharedPtr<SWidget> DisplayWidget;

		// Copy the mouse delegate binding if we want it
		if (InCreateData->bHandleMouseButtonDown)
		{
			MouseButtonDownDelegate = InCreateData->MouseButtonDownDelegate;
		}

		// If the creation data says read only, then it must be read only
		bIsReadOnly = InCreateData->bIsReadOnly || bInIsReadOnly.Get();

		InlineRenameWidget =
			SAssignNew(DisplayWidget, SInlineEditableTextBlock)
			.Text(this, &SHTNBlackboardItem::GetDisplayText)
#if UE_VERSION_OLDER_THAN(5, 0, 0)
			.Font(NameFont)
#endif
			.HighlightText(InCreateData->HighlightText)
			.ToolTipText(this, &SHTNBlackboardItem::GetItemTooltip)
			.OnTextCommitted(this, &SHTNBlackboardItem::OnNameTextCommitted)
			.OnVerifyTextChanged(this, &SHTNBlackboardItem::OnNameTextVerifyChanged)
			.IsSelected( InCreateData->IsRowSelectedDelegate )
			.IsReadOnly(this, &SHTNBlackboardItem::IsReadOnly);

		InCreateData->OnRenameRequest->BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);

		return DisplayWidget.ToSharedRef();
	}

	virtual FText GetItemTooltip() const override
	{
		return ActionPtr.Pin()->GetTooltipDescription();
	}

	virtual void OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit) override
	{
		check(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_BlackboardEntry::StaticGetTypeId());
		
		const FString AsString = *NewText.ToString();

		if (AsString.Len() >= NAME_SIZE)
		{
			UE_LOG(LogBlackboardEditor, Error,
				TEXT("%s is not a valid Blackboard key name. Needs to be shorter than %d characters."), 
				*NewText.ToString(), NAME_SIZE);
			return;
		}

		const TSharedPtr<FEdGraphSchemaAction_BlackboardEntry> BlackboardEntryAction = 
			StaticCastSharedPtr<FEdGraphSchemaAction_BlackboardEntry>(ActionPtr.Pin());

		FName OldName = BlackboardEntryAction->Key.EntryName;
		FName NewName = FName(*AsString);
		if (NewName != OldName)
		{
			TArray<UObject*> ExternalAssetsWithKeyReferences;
			if (!BlackboardEntryAction->bIsNew && BlackboardEntryAction->BlackboardData)
			{
				// Preload HTNs before we transact otherwise they will add objects to 
				// the transaction buffer whether we change them or not.
				// Blueprint regeneration does this in UEdGraphNode::CreatePin.
				LoadReferencerObjects(*(BlackboardEntryAction->BlackboardData), ExternalAssetsWithKeyReferences);
			}

			const FScopedTransaction Transaction(LOCTEXT("BlackboardEntryRenameTransaction", "Rename Blackboard Entry"));
			BlackboardEntryAction->BlackboardData->SetFlags(RF_Transactional);
			BlackboardEntryAction->BlackboardData->Modify();
			BlackboardEntryAction->Key.EntryName = NewName;

			FProperty* const KeysArrayProperty = FindFProperty<FProperty>(UBlackboardData::StaticClass(), 
				GET_MEMBER_NAME_CHECKED(UBlackboardData, Keys));
			FProperty* const NameProperty = FindFProperty<FProperty>(FBlackboardEntry::StaticStruct(), 
				GET_MEMBER_NAME_CHECKED(FBlackboardEntry, EntryName));
			FEditPropertyChain PropertyChain;
			PropertyChain.AddHead(KeysArrayProperty);
			PropertyChain.AddTail(NameProperty);
			PropertyChain.SetActiveMemberPropertyNode(KeysArrayProperty);
			PropertyChain.SetActivePropertyNode(NameProperty);
			BlackboardEntryAction->BlackboardData->PreEditChange(PropertyChain);

			BlackboardEntryAction->Update();

			OnBlackboardKeyChanged.ExecuteIfBound(BlackboardEntryAction->BlackboardData, &BlackboardEntryAction->Key);

			if (!BlackboardEntryAction->bIsNew)
			{
				UpdateExternalBlackboardKeyReferences(OldName, NewName, ExternalAssetsWithKeyReferences);
			}

			FPropertyChangedEvent PropertyChangedEvent(NameProperty, EPropertyChangeType::ValueSet);
			FPropertyChangedChainEvent PropertyChangedChainEvent(PropertyChain, PropertyChangedEvent);
			BlackboardEntryAction->BlackboardData->PostEditChangeChainProperty(PropertyChangedChainEvent);
		}

		BlackboardEntryAction->bIsNew = false;
	}

	void GetBlackboardOwnerClasses(TArray<const UClass*>& BlackboardOwnerClasses)
	{
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			class UClass* Class = *ClassIt;
			if (Class->ImplementsInterface(UBlackboardAssetProvider::StaticClass()))
			{
				BlackboardOwnerClasses.Add(Class);
			}
		}
	}

	void LoadReferencerObjects(const UBlackboardData& InBlackboardData, TArray<UObject*>& OutExternalAssetsWithKeyReferences)
	{
		// Get classes and derived classes which implement UBlackboardAssetProvider.
		TArray<const UClass*> BlackboardOwnerClasses;
		GetBlackboardOwnerClasses(BlackboardOwnerClasses);

		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

		TArray<FName> ReferencerPackages;
		AssetRegistry.GetReferencers(InBlackboardData.GetOutermost()->GetFName(), ReferencerPackages, 
			UE::AssetRegistry::EDependencyCategory::Package, UE::AssetRegistry::EDependencyQuery::Hard);

		if (ReferencerPackages.Num())
		{
			FScopedSlowTask SlowTask((float)ReferencerPackages.Num(), LOCTEXT("UpdatingHTNs", "Updating HTNs"));
			SlowTask.MakeDialog();

			for (const FName& ReferencerPackage : ReferencerPackages)
			{
				TArray<FAssetData> Assets;
				AssetRegistry.GetAssetsByPackageName(ReferencerPackage, Assets);

				for (const FAssetData& Asset : Assets)
				{
					if (BlackboardOwnerClasses.Find(Asset.GetClass()) != INDEX_NONE)
					{
						SlowTask.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("CheckingHTN", "Key renamed, loading {0}"), 
							FText::FromName(Asset.AssetName)));

						UObject* AssetObject = Asset.GetAsset();
						const IBlackboardAssetProvider* BlackboardProvider = Cast<const IBlackboardAssetProvider>(AssetObject);
						if (BlackboardProvider && BlackboardProvider->GetBlackboardAsset() == &InBlackboardData)
						{
							OutExternalAssetsWithKeyReferences.Add(AssetObject);
						}
					}
				}
			}
		}
	}

#define GET_STRUCT_NAME_CHECKED(StructName) \
	((void)sizeof(StructName), TEXT(#StructName))

	void UpdateExternalBlackboardKeyReferences(const FName& OldKey, const FName& NewKey, const TArray<UObject*>& InExternalAssetsWithKeyReferences) const
	{
		for (const UObject* Asset : InExternalAssetsWithKeyReferences)
		{
			// Search all subobjects of this package for FBlackboardKeySelector structs and update as necessary
			TArray<UObject*> Objects;
			GetObjectsWithOuter(Asset->GetOutermost(), Objects);
			for (UObject* const SubObject : Objects)
			{
				for (TFieldIterator<FStructProperty> It(SubObject->GetClass()); It; ++It)
				{
					if (It->GetCPPType(nullptr, CPPF_None).Contains(GET_STRUCT_NAME_CHECKED(FBlackboardKeySelector)))
					{
						FBlackboardKeySelector* PropertyValue = It->ContainerPtrToValuePtr<FBlackboardKeySelector>(SubObject);
						if (PropertyValue && PropertyValue->SelectedKeyName == OldKey)
						{
							SubObject->Modify();
							PropertyValue->SelectedKeyName = NewKey;
						}
					}
				}
			}
		}
	}

#undef GET_STRUCT_NAME_CHECKED

	virtual bool OnNameTextVerifyChanged(const FText& InNewText, FText& OutErrorMessage) override
	{
		check(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_BlackboardEntry::StaticGetTypeId());
		const TSharedPtr<FEdGraphSchemaAction_BlackboardEntry> BlackboardEntryAction = 
			StaticCastSharedPtr<FEdGraphSchemaAction_BlackboardEntry>(ActionPtr.Pin());

		// Error if there is another key with the same name.
		const FString NewTextAsString = InNewText.ToString();
		for (const FBlackboardEntry& Key : BlackboardEntryAction->BlackboardData->Keys)
		{
			if (&BlackboardEntryAction->Key != &Key && Key.EntryName.ToString() == NewTextAsString)
			{
				OutErrorMessage = LOCTEXT("DuplicateKeyWarning", "A key of this name already exists.");
				return false;
			}
		}
		for (const FBlackboardEntry& Key : BlackboardEntryAction->BlackboardData->ParentKeys)
		{
			if (&BlackboardEntryAction->Key != &Key && Key.EntryName.ToString() == NewTextAsString)
			{
				OutErrorMessage = LOCTEXT("DuplicateParentKeyWarning", "An inherited key of this name already exists.");
				return false;
			}
		}

		return true;
	}

	// Create widget for displaying debug information about this blackboard entry
	TSharedRef<SWidget> CreateDebugSlotWidget(const FSlateFontInfo& InFontInfo)
	{
		check(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_BlackboardEntry::StaticGetTypeId());
		TSharedPtr<FEdGraphSchemaAction_BlackboardEntry> BlackboardEntryAction = StaticCastSharedPtr<FEdGraphSchemaAction_BlackboardEntry>(ActionPtr.Pin());

		return SNew(STextBlock)
			.Text(this, &SHTNBlackboardItem::GetDebugTextValue, BlackboardEntryAction)
			.Visibility(this, &SHTNBlackboardItem::GetDebugTextVisibility);
	}

	FText GetDebugTextValue(TSharedPtr<FEdGraphSchemaAction_BlackboardEntry> BlackboardEntry) const
	{
		check(BlackboardEntry.IsValid());
		if (OnGetDebugKeyValue.IsBound() && OnGetDisplayCurrentState.IsBound())
		{
			return OnGetDebugKeyValue.Execute(BlackboardEntry->Key.EntryName, OnGetDisplayCurrentState.Execute());
		}
		
		return FText();
	}

	EVisibility GetDebugTextVisibility() const
	{
		if (OnIsDebuggerReady.IsBound())
		{
			return OnIsDebuggerReady.Execute() ? EVisibility::Visible : EVisibility::Collapsed;
		}

		return EVisibility::Collapsed;
	}

	bool IsReadOnly() const
	{
		if (OnIsDebuggerReady.IsBound())
		{
			return bIsReadOnly || OnIsDebuggerReady.Execute();
		}

		return bIsReadOnly;
	}

private:
	// Delegate used to retrieve debug data to display
	FOnGetDebugKeyValue OnGetDebugKeyValue;

	// Delegate used to determine whether the BT debugger is active
	FOnIsDebuggerReady OnIsDebuggerReady;

	// Delegate used to determine whether the BT debugger displaying the current state
	FOnGetDisplayCurrentState OnGetDisplayCurrentState;

	// Delegate for when a blackboard key changes (added, removed, renamed)
	FOnBlackboardKeyChanged OnBlackboardKeyChanged;

	// Read-only flag
	bool bIsReadOnly;
};

void SHTNBlackboardView::AddReferencedObjects( FReferenceCollector& Collector )
{
	if (BlackboardData != nullptr)
	{
		Collector.AddReferencedObject(BlackboardData);
	}
}

FString SHTNBlackboardView::GetReferencerName() const
{
	return TEXT("SHTNBlackboardView");
}

void SHTNBlackboardView::Construct(const FArguments& InArgs, TSharedRef<FUICommandList> InCommandList, UBlackboardData* InBlackboardData)
{
	OnEntrySelected = InArgs._OnEntrySelected;
	OnGetDebugKeyValue = InArgs._OnGetDebugKeyValue;
	OnIsDebuggerReady = InArgs._OnIsDebuggerReady;
	OnIsDebuggerPaused = InArgs._OnIsDebuggerPaused;
	OnGetDebugTimeStamp = InArgs._OnGetDebugTimeStamp;
	OnGetDisplayCurrentState = InArgs._OnGetDisplayCurrentState;
	OnBlackboardKeyChanged = InArgs._OnBlackboardKeyChanged;

	BlackboardData = InBlackboardData;

	bShowCurrentState = OnGetDisplayCurrentState.IsBound() ? OnGetDisplayCurrentState.Execute() : true;

	const TSharedRef<FUICommandList> CommandList = MakeShared<FUICommandList>();

	CommandList->MapAction(
		FHTNDebuggerCommands::Get().CurrentValues,
		FUIAction(
			FExecuteAction::CreateSP(this, &SHTNBlackboardView::HandleUseCurrentValues),
			FCanExecuteAction::CreateSP(this, &SHTNBlackboardView::IsDebuggerPaused),
			FIsActionChecked::CreateSP(this, &SHTNBlackboardView::IsUsingCurrentValues),
			FIsActionButtonVisible::CreateSP(this, &SHTNBlackboardView::IsDebuggerActive)
		));

	CommandList->MapAction(
		FHTNDebuggerCommands::Get().ValuesOfSelectedNode, 
		FUIAction(
			FExecuteAction::CreateSP(this, &SHTNBlackboardView::HandleUseValuesOfSelectedNode),
			FCanExecuteAction::CreateSP(this, &SHTNBlackboardView::IsDebuggerPaused),
			FIsActionChecked::CreateSP(this, &SHTNBlackboardView::IsUsingValuesOfSelectedNode),
			FIsActionButtonVisible::CreateSP(this, &SHTNBlackboardView::IsDebuggerActive)
		));

	InCommandList->Append(CommandList);

	// Build the debug toolbar.
	FToolBarBuilder ToolbarBuilder(CommandList, FMultiBoxCustomization::None, GetToolbarExtender(InCommandList));
	ToolbarBuilder.BeginSection(TEXT("Debugging"));
	{
		ToolbarBuilder.AddToolBarButton(FHTNDebuggerCommands::Get().CurrentValues);
		ToolbarBuilder.AddToolBarButton(FHTNDebuggerCommands::Get().ValuesOfSelectedNode);
	}
	ToolbarBuilder.EndSection();

	ChildSlot
	[
		SNew(SBorder)
		.Padding(4.0f)
		.BorderImage(FHTNAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					ToolbarBuilder.MakeWidget()
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SHTNBlackboardView::GetDebugTimeStampText)
					.Visibility(this, &SHTNBlackboardView::GetDebuggerToolbarVisibility)
				]
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(GraphActionMenu, SGraphActionMenu, InArgs._IsReadOnly)
				.OnCreateWidgetForAction(this, &SHTNBlackboardView::HandleCreateWidgetForAction)
				.OnCollectAllActions(this, &SHTNBlackboardView::HandleCollectAllActions)
				.OnGetSectionTitle(this, &SHTNBlackboardView::HandleGetSectionTitle)
				.OnActionSelected(this, &SHTNBlackboardView::HandleActionSelected)
				.OnContextMenuOpening(this, &SHTNBlackboardView::HandleContextMenuOpening, InCommandList)
				.OnActionMatchesName(this, &SHTNBlackboardView::HandleActionMatchesName)
				.AlphaSortItems(GetDefault<UEditorPerProjectUserSettings>()->bDisplayBlackboardKeysInAlphabeticalOrder)
				.AutoExpandActionMenu(true)
			]
		]
	];
}

TSharedRef<SWidget> SHTNBlackboardView::HandleCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	return SNew(SHTNBlackboardItem, InCreateData)
		.OnIsDebuggerReady(OnIsDebuggerReady)
		.OnGetDebugKeyValue(OnGetDebugKeyValue)
		.OnGetDisplayCurrentState(this, &SHTNBlackboardView::IsUsingCurrentValues)
		.OnBlackboardKeyChanged(OnBlackboardKeyChanged);
}

void SHTNBlackboardView::HandleCollectAllActions( FGraphActionListBuilderBase& GraphActionListBuilder )
{
	if (BlackboardData)
	{
		for (FBlackboardEntry& ParentKey : BlackboardData->ParentKeys)
		{
			GraphActionListBuilder.AddAction(
				MakeShared<FEdGraphSchemaAction_BlackboardEntry>(BlackboardData, ParentKey, /*bInIsInherited*/true));
		}

		for (FBlackboardEntry& Key : BlackboardData->Keys)
		{
			GraphActionListBuilder.AddAction(
				MakeShared<FEdGraphSchemaAction_BlackboardEntry>(BlackboardData, Key, /*bInIsInherited*/false));
		}
	}
}

FText SHTNBlackboardView::HandleGetSectionTitle(int32 SectionID) const
{
	switch (SectionID)
	{
	case EBlackboardSectionTitles::InheritedKeys:
		return LOCTEXT("InheritedKeysSectionLabel", "Inherited Keys");
	case EBlackboardSectionTitles::Keys:
		return LOCTEXT("KeysSectionLabel", "Keys");
	}

	return FText();
}

void SHTNBlackboardView::HandleActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedActions, 
	ESelectInfo::Type InSelectionType) const
{
	if (!SelectedActions.IsEmpty() && 
		(InSelectionType == ESelectInfo::OnMouseClick || InSelectionType == ESelectInfo::OnKeyPress))
	{
		check(SelectedActions[0]->GetTypeId() == FEdGraphSchemaAction_BlackboardEntry::StaticGetTypeId());
		const TSharedPtr<FEdGraphSchemaAction_BlackboardEntry> BlackboardEntry = 
			StaticCastSharedPtr<FEdGraphSchemaAction_BlackboardEntry>(SelectedActions[0]);
		OnEntrySelected.ExecuteIfBound(&BlackboardEntry->Key, BlackboardEntry->bIsInherited);
	}
}

TSharedPtr<FEdGraphSchemaAction_BlackboardEntry> SHTNBlackboardView::GetSelectedEntryInternal() const
{
	TArray<TSharedPtr<FEdGraphSchemaAction>> SelectedActions;
	GraphActionMenu->GetSelectedActions(SelectedActions);

	if (SelectedActions.Num() > 0)
	{
		check(SelectedActions[0]->GetTypeId() == FEdGraphSchemaAction_BlackboardEntry::StaticGetTypeId());
		return StaticCastSharedPtr<FEdGraphSchemaAction_BlackboardEntry>(SelectedActions[0]);
	}

	return nullptr;
}

FBlackboardEntry* SHTNBlackboardView::GetSelectedEntry(bool& bOutIsInherited) const
{
	if (const TSharedPtr<FEdGraphSchemaAction_BlackboardEntry> Entry = GetSelectedEntryInternal())
	{
		bOutIsInherited = Entry->bIsInherited;
		return &Entry->Key;
	}

	return nullptr;
}

int32 SHTNBlackboardView::GetSelectedEntryIndex(bool& bOutIsInherited) const
{
	if (const TSharedPtr<FEdGraphSchemaAction_BlackboardEntry> Entry = GetSelectedEntryInternal())
	{
		bOutIsInherited = Entry->bIsInherited;

		TArray<FBlackboardEntry>& EntryArray = Entry->bIsInherited ? BlackboardData->ParentKeys : BlackboardData->Keys;
		for (int32 Index = 0; Index < EntryArray.Num(); ++Index)
		{
			if (&Entry->Key == &EntryArray[Index])
			{
				return Index;
			}
		}
	}

	return INDEX_NONE;
}

void SHTNBlackboardView::SetObject(UBlackboardData* InBlackboardData)
{
	BlackboardData = InBlackboardData;
	GraphActionMenu->RefreshAllActions(true);
}

TSharedPtr<SWidget> SHTNBlackboardView::HandleContextMenuOpening(TSharedRef<FUICommandList> ToolkitCommands) const
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, ToolkitCommands);

	FillContextMenu(MenuBuilder);

	return MenuBuilder.MakeWidget();
}

void SHTNBlackboardView::FillContextMenu(FMenuBuilder& MenuBuilder) const
{

}

TSharedPtr<FExtender> SHTNBlackboardView::GetToolbarExtender(TSharedRef<FUICommandList> ToolkitCommands) const
{
	return TSharedPtr<FExtender>();
}

void SHTNBlackboardView::HandleUseCurrentValues()
{
	bShowCurrentState = true;
}

void SHTNBlackboardView::HandleUseValuesOfSelectedNode()
{
	bShowCurrentState = false;
}

FText SHTNBlackboardView::GetDebugTimeStampText() const
{
	FText TimeStampText;

	if (OnGetDebugTimeStamp.IsBound())
	{
		TimeStampText = FText::Format(LOCTEXT("ToolbarTimeStamp", "Time Stamp: {0}"), 
			FText::AsNumber(OnGetDebugTimeStamp.Execute(IsUsingCurrentValues())));
	}

	return TimeStampText;
}

EVisibility SHTNBlackboardView::GetDebuggerToolbarVisibility() const
{
	if (OnIsDebuggerReady.IsBound())
	{
		return OnIsDebuggerReady.Execute() ? EVisibility::Visible : EVisibility::Collapsed;
	}

	return EVisibility::Collapsed;
}

bool SHTNBlackboardView::IsUsingCurrentValues() const
{
	if (OnGetDisplayCurrentState.IsBound())
	{
		return OnGetDisplayCurrentState.Execute() || bShowCurrentState;
	}

	return bShowCurrentState;
}

bool SHTNBlackboardView::IsUsingValuesOfSelectedNode() const
{
	return !IsUsingCurrentValues();
}

bool SHTNBlackboardView::HasSelectedItems() const
{
	bool bIsInherited = false;
	return GetSelectedEntry(bIsInherited) != nullptr;
}

bool SHTNBlackboardView::IsDebuggerActive() const
{
	if (OnIsDebuggerReady.IsBound())
	{
		return OnIsDebuggerReady.Execute();
	}

	return true;
}

bool SHTNBlackboardView::IsDebuggerPaused() const
{
	if (OnIsDebuggerPaused.IsBound())
	{
		return OnIsDebuggerPaused.Execute();
	}

	return true;
}

bool SHTNBlackboardView::HandleActionMatchesName(FEdGraphSchemaAction* InAction, const FName& InName) const
{
	check(InAction->GetTypeId() == FEdGraphSchemaAction_BlackboardEntry::StaticGetTypeId());
	return StaticCast<const FEdGraphSchemaAction_BlackboardEntry*>(InAction)->Key.EntryName == InName;
}

#undef LOCTEXT_NAMESPACE
