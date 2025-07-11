// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNEditor.h"

#include "BlackboardWorldstate.h"
#include "EdGraphSchema_HTN.h"
#include "HTN.h"
#include "HTNAppStyle.h"
#include "HTNDebugger.h"
#include "HTNCommands.h"
#include "HTNEditorModule.h"
#include "HTNEditorModes.h"
#include "HTNEditorTabIds.h"
#include "HTNEditorTabFactories.h"
#include "HTNEditorToolbarBuilder.h"
#include "HTNEditorUtils.h"
#include "HTNGraph.h"
#include "HTNGraphNode.h"
#include "HTNGraphNode_Root.h"
#include "HTNSearch.h"
#include "Nodes/HTNNode_SubNetwork.h"

#include "Algo/Transform.h"
#include "BehaviorTree/BlackboardData.h"
#include "Blackboard/HTNBlackboardDataDetails.h"
#include "Blackboard/SHTNBlackboardEditor.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Editor/UnrealEdEngine.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "IDetailsView.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Modules/ModuleManager.h"
#include "Preferences/UnrealEdOptions.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "SourceCodeNavigation.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UnrealEdGlobals.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"

#define LOCTEXT_NAMESPACE "HTNEditor"

const FName FHTNEditor::ToolkitFName(TEXT("HTNEditor"));
const FName FHTNEditor::HTNMode(TEXT("HTNMode"));
const FName FHTNEditor::BlackboardMode(TEXT("BlackboardMode"));

FHTNEditor::FHTNEditor() :
	CurrentHTN(nullptr),
	CurrentBlackboardData(nullptr)
{
	bCheckDirtyOnAssetSave = true;

#if ENGINE_MAJOR_VERSION < 5
	OnPackageSavedDelegateHandle = UPackage::PackageSavedEvent.AddRaw(this, &FHTNEditor::OnPackageSaved);
#else
	OnPackageSavedDelegateHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FHTNEditor::OnPackageSavedWithContext);
#endif
}

FHTNEditor::~FHTNEditor()
{
#if ENGINE_MAJOR_VERSION < 5
	UPackage::PackageSavedEvent.Remove(OnPackageSavedDelegateHandle);
#else
	UPackage::PackageSavedWithContextEvent.Remove(OnPackageSavedDelegateHandle);
#endif
}

void FHTNEditor::InitHTNEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UHTN* InHTN)
{
	CurrentHTN = InHTN;
	if (CurrentHTN)
	{
		CurrentBlackboardData = CurrentHTN->BlackboardAsset;
	}

	if (!DocumentTracker.IsValid())
	{
		DocumentTracker = MakeShared<FDocumentTracker>();
		DocumentTracker->Initialize(SharedThis(this));
		DocumentTracker->RegisterDocumentFactory(MakeShared<FHTNGraphEditorSummoner>(
			SharedThis(this),
			FHTNGraphEditorSummoner::FOnCreateGraphEditorWidget::CreateSP(this, &FHTNEditor::CreateGraphEditorWidget)
		));
	}

	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShared<FHTNEditorToolbarBuilder>(SharedThis(this));
	}

	TArray<UObject*> ObjectsToEdit;
	if (CurrentHTN) ObjectsToEdit.Add(CurrentHTN);
	if (CurrentBlackboardData) ObjectsToEdit.Add(CurrentBlackboardData);

	const TArray<UObject*>* const EditedObjects = GetObjectsCurrentlyBeingEdited();
	const bool bIsAlreadyEditingObjects = EditedObjects && EditedObjects->Num();
	if (!bIsAlreadyEditingObjects)
	{
		FGraphEditorCommands::Register();
		FHTNCommonCommands::Register();
		FHTNDebuggerCommands::Register();
		FHTNBlackboardCommands::Register();

		// Initialize the editor
		FAssetEditorToolkit::InitAssetEditor(
			Mode,
			InitToolkitHost,
			HTNEditorAppIdentifier,
			/*StandaloneDefaultLayout=*/FTabManager::NewLayout(TEXT("NullLayout"))->AddArea(FTabManager::NewPrimaryArea()),
			/*bCreateDefaultStandaloneMenu=*/true,
			/*bCreateDefaultToolbar=*/true,
			ObjectsToEdit
		);

		BindCommonCommands();

		{
			// Set up the DetailsView
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
			FDetailsViewArgs DetailsViewArgs;
			DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
			DetailsViewArgs.NotifyHook = this;
			DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
			DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
			DetailsView->SetObject(nullptr);
			DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateSP(this, &FHTNEditor::IsPropertyEditable));
			DetailsView->OnFinishedChangingProperties().AddSP(this, &FHTNEditor::OnFinishedChangingProperties);
		}

		{
			// Set up debugger
			Debugger = MakeShared<FHTNDebugger>();
			Debugger->Setup(CurrentHTN, SharedThis(this));
			//Debugger->OnDebuggedBlackboardChanged().AddSP(this, &FHTNEditor::HandleDebuggedBlackboardChanged);
			BindDebuggerToolbarCommands();
		}

		AddApplicationMode(HTNMode, MakeShared<FHTNEditorApplicationMode>(SharedThis(this)));
		AddApplicationMode(BlackboardMode, MakeShared<FHTNBlackboardEditorApplicationMode>(SharedThis(this)));
		
		BlackboardView = SNew(SHTNBlackboardView, GetToolkitCommands(), GetCurrentBlackboardData())
		.OnGetDebugKeyValue(Debugger.Get(), &FHTNDebugger::HandleGetDebugKeyValue)
		.OnIsDebuggerReady(Debugger.Get(), &FHTNDebugger::IsDebuggerReady)
		.OnIsDebuggerPaused(Debugger.Get(), &FHTNDebugger::IsDebuggerPaused)
		//.OnGetDebugTimeStamp(this, &FHTNEditor::HandleGetDebugTimeStamp)
		.OnGetDisplayCurrentState(Debugger.Get(), &FHTNDebugger::IsShowingCurrentState);

		BlackboardEditor = SNew(SHTNBlackboardEditor, GetToolkitCommands(), GetCurrentBlackboardData())
		.OnEntrySelected(this, &FHTNEditor::HandleBlackboardEntrySelected)
		.OnGetDebugKeyValue(Debugger.Get(), &FHTNDebugger::HandleGetDebugKeyValue)
		.OnIsDebuggerReady(Debugger.Get(), &FHTNDebugger::IsDebuggerReady)
		.OnIsDebuggerPaused(Debugger.Get(), &FHTNDebugger::IsDebuggerPaused)
		//.OnGetDebugTimeStamp(this, &FHTNEditor::HandleGetDebugTimeStamp)
		.OnGetDisplayCurrentState(Debugger.Get(), &FHTNDebugger::IsShowingCurrentState)
		.OnBlackboardKeyChanged(this, &FHTNEditor::HandleBlackboardKeyChanged)
		.OnIsBlackboardModeActive(this, &FHTNEditor::IsBlackboardModeActive);
	}
	else
	{
		if (ensure(Debugger.IsValid()))
		{
			Debugger->Setup(CurrentHTN, SharedThis(this));
		}

		for (UObject* const ObjectToEdit : ObjectsToEdit)
		{
			if (!EditedObjects->Contains(ObjectToEdit))
			{
				AddEditingObject(ObjectToEdit);
			}
		}
	}

	// Set the asset we are editing in the details view
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(CurrentHTN);
	}

	if (CurrentHTN)
	{
		SetCurrentMode(HTNMode);
	}
	else if (CurrentBlackboardData)
	{
		SetCurrentMode(BlackboardMode);
	}

	RegenerateMenusAndToolbars();
}

UHTN* FHTNEditor::GetCurrentHTN() const { return CurrentHTN; }
UBlackboardData* FHTNEditor::GetCurrentBlackboardData() const { return CurrentHTN ? CurrentHTN->BlackboardAsset.Get() : CurrentBlackboardData; }
void FHTNEditor::SetCurrentHTN(UHTN* HTN) { CurrentHTN = HTN; }

FName FHTNEditor::GetToolkitFName() const { return ToolkitFName; }
FText FHTNEditor::GetBaseToolkitName() const { return LOCTEXT("AppLabel", "HTN Editor"); }
FText FHTNEditor::GetToolkitName() const
{
	if (const UObject* EditingObject = GetCurrentMode() == HTNMode ? StaticCast<UObject*>(CurrentHTN) : StaticCast<UObject*>(GetCurrentBlackboardData()))
	{
		return FAssetEditorToolkit::GetLabelForObject(EditingObject);
	}

	return FText();
}
FText FHTNEditor::GetToolkitToolTipText() const
{
	if (const UObject* EditingObject = GetCurrentMode() == HTNMode ? StaticCast<UObject*>(CurrentHTN) : StaticCast<UObject*>(GetCurrentBlackboardData()))
	{
		return FAssetEditorToolkit::GetToolTipTextForObject(EditingObject);
	}

	return FText();
}
FString FHTNEditor::GetWorldCentricTabPrefix() const { return LOCTEXT("WorldCentricTabPrefix", "HTN ").ToString(); }
FLinearColor FHTNEditor::GetWorldCentricTabColorScale() const { return FColor::Red; }

void FHTNEditor::FocusWindow(UObject* ObjectToFocusOn)
{
	if (ObjectToFocusOn == CurrentHTN)
	{
		SetCurrentMode(HTNMode);
	}
	else if (ObjectToFocusOn == GetCurrentBlackboardData())
	{
		SetCurrentMode(BlackboardMode);
	}

	FWorkflowCentricApplication::FocusWindow(ObjectToFocusOn);
}

void FHTNEditor::FindInContentBrowser_Execute()
{
	TArray<UObject*> Objects;
	if (IsBlackboardModeActive())
	{
		Objects.Add(GetCurrentBlackboardData());
	}
	else
	{
		Objects.Add(CurrentHTN);
	}
	GEditor->SyncBrowserToObjects(Objects);
}

#if !UE_VERSION_OLDER_THAN(5, 4, 0)
bool FHTNEditor::IncludeAssetInRestoreOpenAssetsPrompt(UObject* Asset) const
{
	// When closing the editor, we want to open the HTN and not the BB.
	// Reopening both would really just open the HTN and then switch to the BB.
	// Reopening whichever we were actually editing would open the BT editor to edit the BB.
	// To avoid that, we always reopen the HTN.
	return Asset && (!Asset->IsA<UBlackboardData>() || !CurrentHTN);
}
#endif

void FHTNEditor::PostUndo(bool bSuccess)
{
	if (bSuccess) RefreshBlackboardViews();
	FAIGraphEditor::PostUndo(bSuccess);
}	

void FHTNEditor::PostRedo(bool bSuccess)
{
	if (bSuccess) RefreshBlackboardViews();
	FAIGraphEditor::PostUndo(bSuccess);
}

void FHTNEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UHTN, BlackboardAsset))
		{
			CurrentBlackboardData = CurrentHTN->BlackboardAsset;
		}

		RefreshBlackboardViews();
	}
}

TWeakPtr<SGraphEditor> FHTNEditor::GetFocusedGraphPtr() const
{
	return UpdateGraphEdPtr;
}

bool FHTNEditor::CanAccessHTNMode() const { return IsValid(CurrentHTN); }
bool FHTNEditor::CanAccessBlackboardMode() const { return IsValid(GetCurrentBlackboardData()); }
bool FHTNEditor::IsBlackboardModeActive() const { return GetCurrentMode() == BlackboardMode; }
bool FHTNEditor::CanEditWithDebuggerActive() const
{
	return !Debugger.IsValid() || !Debugger->IsDebuggerReady();
}

void FHTNEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_HTNEditor", "HTN Editor"));
	DocumentTracker->SetTabManager(InTabManager);
	FWorkflowCentricApplication::RegisterTabSpawners(InTabManager);
}

TSharedRef<SWidget> FHTNEditor::SpawnDetailsWidget()
{
	return
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.HAlign(HAlign_Fill)
		[
			DetailsView.ToSharedRef()
		];
}

TSharedRef<class SWidget> FHTNEditor::SpawnSearchWidget()
{
	return SAssignNew(SearchView, SHTNSearch, SharedThis(this));
}

TSharedRef<SWidget> FHTNEditor::SpawnBlackboardDetailsWidget()
{
	UBlackboardData* const BBData = GetCurrentBlackboardData();

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.NotifyHook = this;
	BlackboardDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	BlackboardDetailsView->RegisterInstancedCustomPropertyLayout(UBlackboardData::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(
		&FHTNBlackboardDataDetails::MakeInstance,
		FOnGetSelectedBlackboardItemIndex::CreateSP(this, &FHTNEditor::GetSelectedBlackboardItemIndex),
		BBData));

	if (BBData)
	{
		BBData->UpdateDeprecatedKeys();
	}

	BlackboardDetailsView->SetObject(BBData);
	BlackboardDetailsView->SetEnabled(TAttribute<bool>(this, &FHTNEditor::CanEditWithDebuggerActive));

	return BlackboardDetailsView.ToSharedRef();
}

TSharedRef<SWidget> FHTNEditor::SpawnBlackboardViewWidget() { return BlackboardView.ToSharedRef(); }
TSharedRef<SWidget> FHTNEditor::SpawnBlackboardEditorWidget() { return BlackboardEditor.ToSharedRef(); }

void FHTNEditor::OnEnableBreakpoint()
{
	for (UObject* const SelectedObject : GetSelectedNodes())
	{
		if (UHTNGraphNode* const SelectedGraphNode = Cast<UHTNGraphNode>(SelectedObject))
		{
			if (SelectedGraphNode->bHasBreakpoint && !SelectedGraphNode->bIsBreakpointEnabled)
			{
				SelectedGraphNode->bIsBreakpointEnabled = true;
				Debugger->OnBreakpointAdded(SelectedGraphNode);
			}
		}
	}
}

bool FHTNEditor::CanEnableBreakpoint() const
{
	for (const UObject* const SelectedObject : GetSelectedNodes())
	{
		if (const UHTNGraphNode* const SelectedGraphNode = Cast<UHTNGraphNode>(SelectedObject))
		{
			if (SelectedGraphNode->bHasBreakpoint && !SelectedGraphNode->bIsBreakpointEnabled)
			{
				return true;
			}
		}
	}

	return false;
}

void FHTNEditor::OnToggleBreakpoint()
{
	for (UObject* const SelectedObject : GetSelectedNodes())
	{
		if (UHTNGraphNode* const SelectedGraphNode = Cast<UHTNGraphNode>(SelectedObject))
		{
			if (SelectedGraphNode->bHasBreakpoint)
			{
				SelectedGraphNode->bHasBreakpoint = false;
				SelectedGraphNode->bIsBreakpointEnabled = false;
				Debugger->OnBreakpointRemoved(SelectedGraphNode);
			}
			else if (SelectedGraphNode->CanPlaceBreakpoints())
			{
				SelectedGraphNode->bHasBreakpoint = true;
				SelectedGraphNode->bIsBreakpointEnabled = true;
				Debugger->OnBreakpointAdded(SelectedGraphNode);
			}
		}
	}
}

bool FHTNEditor::CanToggleBreakpoint() const
{
	for (UObject* const SelectedObject : GetSelectedNodes())
	{
		if (UHTNGraphNode* const SelectedGraphNode = Cast<UHTNGraphNode>(SelectedObject))
		{
			if (SelectedGraphNode->bHasBreakpoint || SelectedGraphNode->CanPlaceBreakpoints())
			{
				return true;
			}
		}
	}

	return false;
}

void FHTNEditor::OnDisableBreakpoint()
{
	for (UObject* const SelectedObject : GetSelectedNodes())
	{
		if (UHTNGraphNode* const SelectedGraphNode = Cast<UHTNGraphNode>(SelectedObject))
		{
			if (SelectedGraphNode->bHasBreakpoint && SelectedGraphNode->bIsBreakpointEnabled)
			{
				SelectedGraphNode->bIsBreakpointEnabled = false;
				Debugger->OnBreakpointRemoved(SelectedGraphNode);
			}
		}
	}
}

bool FHTNEditor::CanDisableBreakpoint() const
{
	for (const UObject* const SelectedObject : GetSelectedNodes())
	{
		if (const UHTNGraphNode* const SelectedGraphNode = Cast<UHTNGraphNode>(SelectedObject))
		{
			if (SelectedGraphNode->bHasBreakpoint && SelectedGraphNode->bIsBreakpointEnabled)
			{
				return true;
			}
		}
	}

	return false;
}

void FHTNEditor::OnAddBreakpoint()
{
	for (UObject* const SelectedObject : GetSelectedNodes())
	{
		if (UHTNGraphNode* const SelectedGraphNode = Cast<UHTNGraphNode>(SelectedObject))
		{
			if (!SelectedGraphNode->bHasBreakpoint && SelectedGraphNode->CanPlaceBreakpoints())
			{
				SelectedGraphNode->bHasBreakpoint = true;
				SelectedGraphNode->bIsBreakpointEnabled = true;
				Debugger->OnBreakpointAdded(SelectedGraphNode);
			}
		}
	}
}

bool FHTNEditor::CanAddBreakpoint() const
{
	for (const UObject* const SelectedObject : GetSelectedNodes())
	{
		if (const UHTNGraphNode* const SelectedGraphNode = Cast<UHTNGraphNode>(SelectedObject))
		{
			if (!SelectedGraphNode->bHasBreakpoint && SelectedGraphNode->CanPlaceBreakpoints())
			{
				return true;
			}
		}
	}

	return false;
}

void FHTNEditor::OnRemoveBreakpoint()
{
	for (UObject* const SelectedObject : GetSelectedNodes())
	{
		if (UHTNGraphNode* const SelectedGraphNode = Cast<UHTNGraphNode>(SelectedObject))
		{
			if (SelectedGraphNode->bHasBreakpoint)
			{
				SelectedGraphNode->bHasBreakpoint = false;
				SelectedGraphNode->bIsBreakpointEnabled = false;
				Debugger->OnBreakpointRemoved(SelectedGraphNode);
			}
		}
	}
}

bool FHTNEditor::CanRemoveBreakpoint() const
{
	for (const UObject* const SelectedObject : GetSelectedNodes())
	{
		if (const UHTNGraphNode* const SelectedGraphNode = Cast<UHTNGraphNode>(SelectedObject))
		{
			if (SelectedGraphNode->bHasBreakpoint)
			{
				return true;
			}
		}
	}

	return false;
}

void FHTNEditor::RestoreHTN()
{
	struct Local
	{
		static UHTNGraph& EnsureHTNGraph(UHTN& HTN, bool& bOutIsNewGraph)
		{
			bOutIsNewGraph = false;
			if (UHTNGraph* const Graph = Cast<UHTNGraph>(HTN.HTNGraph))
			{
				Graph->OnLoaded();
				return *Graph;
			}

			HTN.HTNGraph = FBlueprintEditorUtils::CreateNewGraph(&HTN, TEXT("Hierarchical Task Network"), UHTNGraph::StaticClass(), UEdGraphSchema_HTN::StaticClass());
			UHTNGraph* const Graph = Cast<UHTNGraph>(HTN.HTNGraph);
			check(Graph);

			const UEdGraphSchema* const Schema = Graph->GetSchema();
			check(Schema);
			Schema->CreateDefaultNodesForGraph(*Graph);

			Graph->OnCreated();

			bOutIsNewGraph = true;
			return *Graph;
		}
	};
	
	if (!CurrentHTN)
	{
		return;
	}

	bool bHadToCreateNewGraph = false;
	UHTNGraph& Graph = Local::EnsureHTNGraph(*CurrentHTN, bHadToCreateNewGraph);
	Graph.Initialize();
	
	TSharedPtr<SDockTab> DocumentTab = DocumentTracker->OpenDocument(
		FTabPayload_UObject::Make(&Graph), 
		bHadToCreateNewGraph ? FDocumentTracker::OpenNewDocument : FDocumentTracker::RestorePreviousDocument
	);

	// Restore location and zoom level
	const int32 EditedDocumentInfoIndex = CurrentHTN->LastEditedDocuments.FindLastByPredicate([&](const FEditedDocumentInfo& Info)
	{
		return Info.EditedObjectPath == &Graph;
	});
	if (EditedDocumentInfoIndex != INDEX_NONE)
	{
		const FEditedDocumentInfo& EditedDocumentInfo = CurrentHTN->LastEditedDocuments[EditedDocumentInfoIndex];
		const TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(DocumentTab->GetContent());
		GraphEditor->SetViewLocation(EditedDocumentInfo.SavedViewOffset, EditedDocumentInfo.SavedZoomAmount);
	}

	if (bHadToCreateNewGraph)
	{
		RefreshBlackboardViews();
	}

	Graph.UpdateAsset();
}

void FHTNEditor::SaveEditedObjectState()
{
	if (ensure(CurrentHTN))
	{
		CurrentHTN->LastEditedDocuments.Reset();
	}

	if (ensure(DocumentTracker.IsValid()))
	{
		DocumentTracker->SaveAllState();
	}
}

TSharedRef<SGraphEditor> FHTNEditor::CreateGraphEditorWidget(UEdGraph* InGraph)
{
	check(InGraph);

	// Ensure commands like "delete", "copy" etc. are registered.
	if (!GraphEditorCommands.IsValid())
	{
		CreateCommandList();

		// Debug actions
		GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AddBreakpoint,
			FExecuteAction::CreateSP(this, &FHTNEditor::OnAddBreakpoint),
			FCanExecuteAction::CreateSP(this, &FHTNEditor::CanAddBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FHTNEditor::CanAddBreakpoint)
		);

		GraphEditorCommands->MapAction(FGraphEditorCommands::Get().RemoveBreakpoint,
			FExecuteAction::CreateSP(this, &FHTNEditor::OnRemoveBreakpoint),
			FCanExecuteAction::CreateSP(this, &FHTNEditor::CanRemoveBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FHTNEditor::CanRemoveBreakpoint)
		);

		GraphEditorCommands->MapAction(FGraphEditorCommands::Get().EnableBreakpoint,
			FExecuteAction::CreateSP(this, &FHTNEditor::OnEnableBreakpoint),
			FCanExecuteAction::CreateSP(this, &FHTNEditor::CanEnableBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FHTNEditor::CanEnableBreakpoint)
		);

		GraphEditorCommands->MapAction(FGraphEditorCommands::Get().DisableBreakpoint,
			FExecuteAction::CreateSP(this, &FHTNEditor::OnDisableBreakpoint),
			FCanExecuteAction::CreateSP(this, &FHTNEditor::CanDisableBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FHTNEditor::CanDisableBreakpoint)
		);

		GraphEditorCommands->MapAction(FGraphEditorCommands::Get().ToggleBreakpoint,
			FExecuteAction::CreateSP(this, &FHTNEditor::OnToggleBreakpoint),
			FCanExecuteAction::CreateSP(this, &FHTNEditor::CanToggleBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FHTNEditor::CanToggleBreakpoint)
		);
	}

	SGraphEditor::FGraphEditorEvents GraphEditorEvents;
	GraphEditorEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FHTNEditor::OnSelectedNodesChanged);
	GraphEditorEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FHTNEditor::OnNodeDoubleClicked);
	GraphEditorEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FHTNEditor::OnNodeTitleCommitted);

	// Make title bar
	const TSharedRef<SWidget> TitleBarWidget =
		SNew(SBorder)
		.BorderImage(FHTNAppStyle::GetBrush(TEXT("Graph.TitleBackground")))
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("HTNGraphLabel", "Hierarchical Task Network"))
				.TextStyle(FHTNAppStyle::Get(), TEXT("GraphBreadcrumbButtonText"))
			]
		];

	// Make full graph editor
	const bool bGraphIsEditable = InGraph->bEditable;
	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(this, &FHTNEditor::IsInEditingMode, bGraphIsEditable)
		.Appearance(this, &FHTNEditor::GetGraphAppearance)
		.TitleBar(TitleBarWidget)
		.GraphToEdit(InGraph)
		.GraphEvents(GraphEditorEvents);
}

FGraphAppearanceInfo FHTNEditor::GetGraphAppearance() const
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "HIERARCHICAL TASK NETWORK");

	if (Debugger.IsValid() && !Debugger->IsDebuggerRunning())
	{
		AppearanceInfo.PIENotifyText = LOCTEXT("InactiveLabel", "INACTIVE");
	}
	else if (FHTNDebugger::IsPlaySessionPaused())
	{
		AppearanceInfo.PIENotifyText = LOCTEXT("PausedLabel", "PAUSED");
	}

	return AppearanceInfo;
}

void FHTNEditor::OnNodeDoubleClicked(UEdGraphNode* GraphNode)
{
	UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode);
	if (!HTNGraphNode)
	{
		return;
	}

	UHTNNode* const HTNNode = Cast<UHTNNode>(HTNGraphNode->NodeInstance);
	if (!HTNNode)
	{
		return;
	}

	UAssetEditorSubsystem* const AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		return;
	}

	const UHTNComponent* const DebuggedHTNComponent = Debugger.IsValid() && Debugger->IsDebuggerReady() ? 
		Debugger->GetDebuggedComponent() : nullptr;
	
	// Open whatever custom asset the node wants to open
	if (UObject* const Asset = HTNNode->GetAssetToOpenOnDoubleClick(DebuggedHTNComponent))
	{
		AssetEditorSubsystem->OpenEditorForAsset(Asset);
		AssetEditorSubsystem->FindEditorForAsset(Asset, /*bFocusIfOpen=*/true);
	}
	else if (const UClass* const NodeClass = HTNNode->GetClass())
	{
		// Open the Blueprint node's class
		if (NodeClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
		{
			UPackage* const Package = NodeClass->GetOuterUPackage();
			const FString ClassName = NodeClass->GetName().LeftChop(2);
			if (UBlueprint* const Blueprint = FindObject<UBlueprint>(Package, *ClassName))
			{
				AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
			}
		}
		// Open the C++ node's class
		else if (ensure(GUnrealEd) && GUnrealEd->GetUnrealEdOptions()->IsCPPAllowed())
		{
			if (FSourceCodeNavigation::CanNavigateToClass(NodeClass))
			{
				FSourceCodeNavigation::NavigateToClass(NodeClass);
			}
		}
	}
}

void FHTNEditor::OnSearch()
{
	if (TabManager.IsValid())
	{
		TabManager->TryInvokeTab(FHTNEditorTabIds::SearchID);
	}

	if (SearchView.IsValid())
	{
		SearchView->FocusForUse();
	}
}

void FHTNEditor::OnSelectedNodesChanged(const TSet<UObject*>& NewSelection)
{
	if (Debugger.IsValid())
	{
		Debugger->OnSelectedNodesChanged(NewSelection);
	}
	
	if (DetailsView.IsValid())
	{
		const TArray<UObject*> SelectedObjects = FHTNEditorUtils::GetSelectionForPropertyEditor(NewSelection);
		if (SelectedObjects.Num())
		{
			DetailsView->SetObjects(SelectedObjects);
		}
		else
		{
			UHTNGraph* const MyGraph = Cast<UHTNGraph>(CurrentHTN->HTNGraph);
			UHTNGraphNode_Root* const RootNode = MyGraph ? MyGraph->FindRootNode() : nullptr;
			DetailsView->SetObject(RootNode);
		}
	}
}

void FHTNEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged)
	{
		static const FText TransactionTitle = FText::FromString(FString(TEXT("Rename Node")));
		const FScopedTransaction Transaction(TransactionTitle);
		NodeBeingChanged->Modify();
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

bool FHTNEditor::IsInEditingMode(bool bGraphIsEditable) const { return bGraphIsEditable && IsPIENotSimulating(); }

void FHTNEditor::BindCommonCommands()
{
	ToolkitCommands->MapAction(FHTNCommonCommands::Get().SearchHTN,
		FExecuteAction::CreateSP(this, &FHTNEditor::OnSearch));
}

void FHTNEditor::BindDebuggerToolbarCommands()
{
	const FHTNDebuggerCommands& Commands = FHTNDebuggerCommands::Get();
	const TSharedRef<FHTNDebugger> DebuggerSharedRef = Debugger.ToSharedRef();

	/*
	ToolkitCommands->MapAction(
		Commands.BackOver,
		FExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::StepBackOver),
		FCanExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::CanStepBackOver));

	ToolkitCommands->MapAction(
		Commands.BackInto,
		FExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::StepBackInto),
		FCanExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::CanStepBackInto));

	ToolkitCommands->MapAction(
		Commands.ForwardInto,
		FExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::StepForwardInto),
		FCanExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::CanStepForwardInto));

	ToolkitCommands->MapAction(
		Commands.ForwardOver,
		FExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::StepForwardOver),
		FCanExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::CanStepForwardOver));

	ToolkitCommands->MapAction(
		Commands.StepOut,
		FExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::StepOut),
		FCanExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::CanStepOut));
	*/
	
	ToolkitCommands->MapAction(
		Commands.PausePlaySession,
		FExecuteAction::CreateStatic(&FHTNDebugger::PausePlaySession),
		FCanExecuteAction::CreateStatic(&FHTNDebugger::IsPlaySessionRunning),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic(&FHTNDebugger::IsPlaySessionRunning));

	ToolkitCommands->MapAction(
		Commands.ResumePlaySession,
		FExecuteAction::CreateStatic(&FHTNDebugger::ResumePlaySession),
		FCanExecuteAction::CreateStatic(&FHTNDebugger::IsPlaySessionPaused),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic(&FHTNDebugger::IsPlaySessionPaused));

	ToolkitCommands->MapAction(
		Commands.StopPlaySession,
		FExecuteAction::CreateStatic(&FHTNDebugger::StopPlaySession));
}

void FHTNEditor::OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor)
{
	UpdateGraphEdPtr = InGraphEditor;
	OnSelectedNodesChanged(InGraphEditor->GetSelectedNodes());
}

void FHTNEditor::RegisterToolbarTabSpawner(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FHTNEditor::SaveAsset_Execute()
{
	if (CurrentHTN)
	{
		if (UHTNGraph* const HTNGraph = Cast<UHTNGraph>(CurrentHTN->HTNGraph))
		{
			HTNGraph->OnSave();
		}
	}

	IHTNEditor::SaveAsset_Execute();
}

int32 FHTNEditor::GetSelectedBlackboardItemIndex(bool& bIsInherited) const
{
	bIsInherited = false;
	return BlackboardEditor.IsValid() ? BlackboardEditor->GetSelectedEntryIndex(bIsInherited) : INDEX_NONE;
}

void FHTNEditor::HandleBlackboardEntrySelected(const FBlackboardEntry* BlackboardEntry, bool bIsInherited)
{
	if (ensure(BlackboardDetailsView.IsValid()))
	{
		BlackboardDetailsView->SetObject(GetCurrentBlackboardData(), /*bForceRefresh=*/true);
	}
}

void FHTNEditor::HandleBlackboardKeyChanged(UBlackboardData* InBlackboardData, FBlackboardEntry* InKey)
{
	if (ensure(BlackboardView.IsValid()))
	{
		// re-set object in blackboard view to keep it up to date
		BlackboardView->SetObject(InBlackboardData);
	}
}

void FHTNEditor::RefreshBlackboardViews()
{
	if (BlackboardView.IsValid()) BlackboardView->SetObject(GetCurrentBlackboardData());
	if (BlackboardEditor.IsValid()) BlackboardEditor->SetObject(GetCurrentBlackboardData());
}

bool FHTNEditor::IsPropertyEditable() const
{
	if (IsPIESimulating())
	{
		return false;
	}

	const TSharedPtr<SGraphEditor> FocusedGraphEd = UpdateGraphEdPtr.Pin();
	return FocusedGraphEd.IsValid() && FocusedGraphEd->GetCurrentGraph() && FocusedGraphEd->GetCurrentGraph()->bEditable;
}

void FHTNEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property)
	{
		const FName ChangedPropertyName = PropertyChangedEvent.Property->GetFName();
		if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UHTN, BlackboardAsset))
		{
			RefreshBlackboardViews();
		}
		else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UHTNNode_SubNetwork, HTN))
		{
			UHTNGraph* const MyGraph = Cast<UHTNGraph>(CurrentHTN->HTNGraph);
			MyGraph->UpdateAsset();
		}
	}
	
	CurrentHTN->HTNGraph->GetSchema()->ForceVisualizationCacheClear();
}

#if ENGINE_MAJOR_VERSION < 5
void FHTNEditor::OnPackageSaved(const FString& PackageFileName, UObject* Outer)
#else
void FHTNEditor::OnPackageSavedWithContext(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext Context)
#endif
{
	if (UHTNGraph* const MyGraph = CurrentHTN ? Cast<UHTNGraph>(CurrentHTN->HTNGraph) : nullptr)
	{
		MyGraph->UpdateAsset();
	}
}

FText FHTNEditor::GetLocalizedModeDescription(FName Mode)
{
	static TMap<FName, FText> LocModes;
	if (LocModes.Num() == 0)
	{
		LocModes.Add(HTNMode, LOCTEXT("HTNMode", "Hierarchical Task Network"));
		LocModes.Add(BlackboardMode, LOCTEXT("BlackboardMode", "Blackboard"));
	}

	check(Mode != NAME_None);
	const FText* const Description = LocModes.Find(Mode);
	check(Description);
	return *Description;
}

bool FHTNEditor::IsPIESimulating() { return GEditor->bIsSimulatingInEditor || GEditor->PlayWorld; }
bool FHTNEditor::IsPIENotSimulating() { return !GEditor->bIsSimulatingInEditor && !GEditor->PlayWorld; }

#undef LOCTEXT_NAMESPACE