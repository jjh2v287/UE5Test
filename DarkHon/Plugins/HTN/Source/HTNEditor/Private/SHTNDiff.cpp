// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "SHTNDiff.h"

#include "HTN.h"
#include "HTNEditorUtils.h"
#include "HTNGraphNode.h"
#include "HTNGraphNode_Decorator.h"
#include "HTNGraphNode_Service.h"
#include "HTNAppStyle.h"

#include "Containers/BitArray.h"
#include "Containers/Set.h"
#include "Containers/UnrealString.h"
#include "DiffResults.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphUtilities.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/InputChord.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/Views/ITypedTableView.h"
#include "GraphDiffControl.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformCrt.h"
#include "IDetailsView.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "Layout/Children.h"
#include "Layout/Margin.h"
#include "Layout/Visibility.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Attribute.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "SlateOptMacros.h"
#include "SlotBase.h"
#include "Styling/SlateColor.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

#if !UE_VERSION_OLDER_THAN(5, 0, 0)
#include "DetailsViewArgs.h"
#endif

class ITableRow;
class STableViewBase;
class SWidget;
class UObject;
struct FSlateBrush;

#define LOCTEXT_NAMESPACE "SHTNDiff"

//////////////////////////////////////////////////////////////////////////
// FHTNDiffResultItem

struct FHTNDiffResultItem : public TSharedFromThis<FHTNDiffResultItem>
{
	FHTNDiffResultItem(const FDiffSingleResult& InResult): Result(InResult){}

	TSharedRef<SWidget>	GenerateWidget() const
	{
#if UE_VERSION_OLDER_THAN(5, 1, 0)
		FSlateColor Color = Result.DisplayColor;
#else
		FSlateColor Color = Result.GetDisplayColor();
#endif
		FText Text = Result.DisplayString;
		FText ToolTip = Result.ToolTip;
		if (Text.IsEmpty())
		{
			Text = LOCTEXT("DIF_UnknownDiff", "Unknown Diff");
			ToolTip = LOCTEXT("DIF_Confused", "There is an unspecified difference");
		}
		return SNew(STextBlock)
			.ToolTipText(ToolTip)
			.ColorAndOpacity(Color)
			.Text(Text);
	}

	const FDiffSingleResult Result;
};


//////////////////////////////////////////////////////////////////////////
// FDiffListCommands

class FDiffListCommands : public TCommands<FDiffListCommands>
{
public:
	// Constructor
	FDiffListCommands() 
		: TCommands<FDiffListCommands>("HTNDiffList", LOCTEXT("Diff", "HTN Diff"), NAME_None, FHTNAppStyleGetAppStyleSetName())
	{
	}

	// Initialize commands
	virtual void RegisterCommands() override
	{
		UI_COMMAND(Previous, "Prev", "Go to previous difference", EUserInterfaceActionType::Button, FInputChord(EKeys::F7, EModifierKey::Control));
		UI_COMMAND(Next, "Next", "Go to next difference", EUserInterfaceActionType::Button, FInputChord(EKeys::F7));
		UI_COMMAND(DiffAsText, "Diff as Text", "Open as text in the default diff tool", EUserInterfaceActionType::Button, FInputChord(EKeys::D, EModifierKey::Control));
	}

	// Go to previous difference
	TSharedPtr<FUICommandInfo> Previous;

	// Go to next difference
	TSharedPtr<FUICommandInfo> Next;

	// pen as text in the default diff tool
	TSharedPtr<FUICommandInfo> DiffAsText;
};

//////////////////////////////////////////////////////////////////////////
// SHTNDiff

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SHTNDiff::Construct( const FArguments& InArgs )
{
	FoundDiffs = MakeShared<TArray<FDiffSingleResult>>();

	FDiffListCommands::Register();

	PanelOld.HTN = InArgs._HTNOld;
	PanelNew.HTN = InArgs._HTNNew;

	PanelOld.RevisionInfo = InArgs._OldRevision;
	PanelNew.RevisionInfo = InArgs._NewRevision;

	PanelOld.bShowAssetName = InArgs._ShowAssetNames;
	PanelNew.bShowAssetName = InArgs._ShowAssetNames;

	OpenInDefaultDiffTool = InArgs._OpenInDefaultDiffTool;

	this->ChildSlot
	[	
		SNew(SBorder)
		.BorderImage(FHTNAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Content()
		[
			SNew(SSplitter)
			+SSplitter::Slot()
			.Value(0.2f)
			[
				SNew(SBorder)
				[
					GenerateDiffListWidget()
				]
			]
			+SSplitter::Slot()
			.Value(0.8f)
			[
				GenerateDiffPanels()
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SHTNDiff::OnOpenInDefaultDiffTool()
{
	OpenInDefaultDiffTool.ExecuteIfBound(PanelOld.HTN, PanelNew.HTN);
}

TSharedRef<SWidget> SHTNDiff::GenerateDiffListWidget()
{
	BuildDiffSourceArray();
	if (DiffListSource.Num() > 0)
	{
		struct FSortDiff
		{
			bool operator()(const FSharedDiffOnGraph& A, const FSharedDiffOnGraph& B) const
			{
				return A->Result.Diff < B->Result.Diff;
			}
		};
		DiffListSource.Sort(FSortDiff());

		// Map commands through UI
		const FDiffListCommands& Commands = FDiffListCommands::Get();
		KeyCommands = MakeShared<FUICommandList>();
		KeyCommands->MapAction(Commands.Previous, FExecuteAction::CreateSP(this, &SHTNDiff::PrevDiff));
		KeyCommands->MapAction(Commands.Next, FExecuteAction::CreateSP(this, &SHTNDiff::NextDiff));
		KeyCommands->MapAction(Commands.DiffAsText, FExecuteAction::CreateSP(this, &SHTNDiff::OnOpenInDefaultDiffTool));

		FToolBarBuilder ToolbarBuilder(KeyCommands.ToSharedRef(), FMultiBoxCustomization::None);
		ToolbarBuilder.AddToolBarButton(Commands.Previous, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FHTNAppStyleGetAppStyleSetName(), "BlueprintDif.PrevDiff"));
		ToolbarBuilder.AddToolBarButton(Commands.Next, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FHTNAppStyleGetAppStyleSetName(), "BlueprintDif.NextDiff"));
		ToolbarBuilder.AddToolBarButton(Commands.DiffAsText, NAME_None, TAttribute<FText>(), TAttribute<FText>());

		return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1.f)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.Padding(0.f)
			.AutoHeight()
			[
				ToolbarBuilder.MakeWidget()
			]
			+SVerticalBox::Slot()
			.Padding(1.f)
			.FillHeight(1.f)
			[
				SAssignNew(DiffList, SListView<FSharedDiffOnGraph>)
				.ListItemsSource(&DiffListSource)
				.OnGenerateRow(this, &SHTNDiff::OnGenerateRow)
				.SelectionMode(ESelectionMode::Single)
				.OnSelectionChanged(this, &SHTNDiff::OnSelectionChanged)
			]
		];
	}
	else
	{
		return SNew(SBorder).Visibility(EVisibility::Hidden);
	}
}

TSharedRef<SWidget> SHTNDiff::GenerateDiffPanels()
{
	FToolBarBuilder ToolbarBuilder(nullptr, FMultiBoxCustomization::None);
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SHTNDiff::OnToggleLockView)),
		NAME_None,
		LOCTEXT("LockGraphsLabel", "Lock/Unlock"),
		LOCTEXT("LockGraphsTooltip", "Force all graph views to change together, or allow independent scrolling/zooming"),
		TAttribute<FSlateIcon>(this, &SHTNDiff::GetLockViewImage));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SHTNDiff::OnToggleSplitViewMode)),
		NAME_None,
		LOCTEXT("SplitGraphsModeLabel", "Vertical/Horizontal"),
		LOCTEXT("SplitGraphsModeLabelTooltip", "Toggles the split view of graphs between vertical and horizontal"),
		TAttribute<FSlateIcon>(this, &SHTNDiff::GetSplitViewModeImage)
	);

	const TSharedRef<SHorizontalBox> DefaultEmptyPanel = SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("HTNDifGraphsToolTip", "Select Graph to Diff"))
	];

	const TSharedRef<SWidget> Result = SNew(SVerticalBox)
	+SVerticalBox::Slot()
	.Padding(0.f)
	.AutoHeight()
	[
		ToolbarBuilder.MakeWidget()
	]
	+SVerticalBox::Slot()
	.Padding(1.f)
	.FillHeight(1.f)
	[
		// Diff Window
		SNew(SSplitter)
		.Orientation(Orient_Vertical)
		+ SSplitter::Slot()
		.Value(0.8f)
		[
			// The two diff panels, horizontally or vertically split
			SAssignNew(DiffGraphSplitter, SSplitter)
			.Orientation(bVerticalSplitGraphMode ? Orient_Vertical : Orient_Horizontal)
			+SSplitter::Slot()
			.Value(0.5f)
			[
				// Left Diff
#if UE_VERSION_OLDER_THAN(5, 1, 0)
				PanelOld.CreateGraphWidget(PanelNew.HTN->HTNGraph)
#else
				PanelOld.CreateGraphWidget(FoundDiffs)
#endif
			]
			+SSplitter::Slot()
			.Value(0.5f)
			[
				// Right diff
#if UE_VERSION_OLDER_THAN(5, 1, 0)
				PanelNew.CreateGraphWidget(PanelOld.HTN->HTNGraph)
#else
				PanelNew.CreateGraphWidget(FoundDiffs)
#endif
			]
		]
		+ SSplitter::Slot()
		.Value(0.2f)
		[
			// Details panels, always horizontally split
			SNew(SSplitter)
			+ SSplitter::Slot()
			[
				PanelOld.CreateDetailsWidget()
			]
			+ SSplitter::Slot()
			[
				PanelNew.CreateDetailsWidget()
			]
		]
	];

	ResetGraphEditors();
	return Result;
}

void SHTNDiff::BuildDiffSourceArray()
{
	FoundDiffs->Empty();
	FGraphDiffControl::DiffGraphs(PanelOld.HTN->HTNGraph, PanelNew.HTN->HTNGraph, *FoundDiffs);

	DiffListSource.Empty();
	for (auto DiffIt(FoundDiffs->CreateConstIterator()); DiffIt; ++DiffIt)
	{
		DiffListSource.Add(MakeShared<FHTNDiffResultItem>(*DiffIt));
	}
}

void SHTNDiff::NextDiff()
{
	int32 Index = (GetCurrentDiffIndex() + 1) % DiffListSource.Num();
	DiffList->SetSelection(DiffListSource[Index]);
}

void SHTNDiff::PrevDiff()
{
	int32 Index = GetCurrentDiffIndex();
	if (Index == 0)
	{
		Index = DiffListSource.Num() - 1;
	}
	else
	{
		Index = (Index - 1) % DiffListSource.Num();
	}
	DiffList->SetSelection(DiffListSource[Index]);
}

int32 SHTNDiff::GetCurrentDiffIndex() 
{
	auto Selected = DiffList->GetSelectedItems();
	if (Selected.Num() == 1)
	{	
		int32 Index = 0;
		for (auto It(DiffListSource.CreateIterator());It;++It,Index++)
		{
			if (*It == Selected[0])
			{
				return Index;
			}
		}
	}
	return 0;
}

TSharedRef<ITableRow> SHTNDiff::OnGenerateRow(FSharedDiffOnGraph Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FSharedDiffOnGraph>, OwnerTable)
		.Content()
		[
			Item->GenerateWidget()
		];
}

void SHTNDiff::OnSelectionChanged(FSharedDiffOnGraph Item, ESelectInfo::Type SelectionType)
{
	if (!Item.IsValid())
	{
		return;
	}

	const auto SafeClearSelection = [](TWeakPtr<SGraphEditor> GraphEditor)
	{
		if (const TSharedPtr<SGraphEditor> GraphEditorPtr = GraphEditor.Pin())
		{
			GraphEditorPtr->ClearSelectionSet();
		}
	};

	// Focus the graph onto the diff that was clicked on
	FDiffSingleResult Result = Item->Result;
	if (Result.Pin1)
	{
		SafeClearSelection(PanelNew.GraphEditor);
		SafeClearSelection(PanelOld.GraphEditor);

		const auto FocusPin = [this](UEdGraphPin* InPin)
		{
			if (InPin)
			{
				LastPinTarget = InPin;

				UEdGraph* NodeGraph = InPin->GetOwningNode()->GetGraph();
				SGraphEditor* NodeGraphEditor = GetGraphEditorForGraph(NodeGraph);
				NodeGraphEditor->JumpToPin(InPin);
			}
		};

		FocusPin(Result.Pin1);
		FocusPin(Result.Pin2);
	}
	else if (Result.Node1)
	{
		SafeClearSelection(PanelNew.GraphEditor);
		SafeClearSelection(PanelOld.GraphEditor);

		const auto FocusNode = [this](UEdGraphNode* InNode)
		{
			if (!InNode)
			{
				return;
			}
			
			UEdGraph* NodeGraph = InNode->GetGraph();
			if (!NodeGraph)
			{
				return;
			}
			
			SGraphEditor* NodeGraphEditor = GetGraphEditorForGraph(NodeGraph);
			if (!NodeGraphEditor)
			{
				return;
			}

			UHTNGraphNode* HTNGraphNode = Cast<UHTNGraphNode>(InNode);
			if (HTNGraphNode && HTNGraphNode->bIsSubNode)
			{
				// This is a sub-node, we need to find our parent node in the graph
#if UE_VERSION_OLDER_THAN(5, 0, 0)
				UEdGraphNode** ParentNodePtr =
#else 
				TObjectPtr<UEdGraphNode>* ParentNodePtr =
#endif
				NodeGraph->Nodes.FindByPredicate([&](UEdGraphNode* PotentialParentNode) -> bool
				{
					if (UHTNGraphNode* const HTNPotentialParentNode = Cast<UHTNGraphNode>(PotentialParentNode))
					{
						return 
							HTNPotentialParentNode->Decorators.Contains(HTNGraphNode) || 
							HTNPotentialParentNode->Services.Contains(HTNGraphNode);
					}

					return false;
				});

				// We need to call JumpToNode on the parent node, and then SetNodeSelection on the sub-node
				// as JumpToNode doesn't work for sub-nodes
				if (ParentNodePtr)
				{
					check(InNode->GetGraph() == (*ParentNodePtr)->GetGraph());
					NodeGraphEditor->JumpToNode(*ParentNodePtr, false, false);
				}
				NodeGraphEditor->SetNodeSelection(InNode, true);
			}
			else
			{
				NodeGraphEditor->JumpToNode(InNode, false);
			}
		};

		FocusNode(Result.Node1);
		FocusNode(Result.Node2);
	}
}

SGraphEditor* SHTNDiff::GetGraphEditorForGraph(UEdGraph* Graph) const
{
	if (PanelOld.GraphEditor.Pin()->GetCurrentGraph() == Graph)
	{
		return PanelOld.GraphEditor.Pin().Get();
	}
	else if (PanelNew.GraphEditor.Pin()->GetCurrentGraph() == Graph)
	{
		return PanelNew.GraphEditor.Pin().Get();
	}

	checkNoEntry();
	return nullptr;
}

void SHTNDiff::OnToggleLockView()
{
	bLockViews = !bLockViews;
	ResetGraphEditors();
}

void SHTNDiff::OnToggleSplitViewMode()
{
	bVerticalSplitGraphMode = !bVerticalSplitGraphMode;

	if (SSplitter* const Splitter = DiffGraphSplitter.Get())
	{
		Splitter->SetOrientation(bVerticalSplitGraphMode ? Orient_Vertical : Orient_Horizontal);
	}
}

FSlateIcon SHTNDiff::GetLockViewImage() const
{
	return FSlateIcon(FHTNAppStyleGetAppStyleSetName(), bLockViews ? "Icons.Lock" : "Icons.Unlock");
}

FSlateIcon SHTNDiff::GetSplitViewModeImage() const
{
	return FSlateIcon(FHTNAppStyle::GetAppStyleSetName(),
		bVerticalSplitGraphMode ? "BlueprintDif.VerticalDiff.Small" : "BlueprintDif.HorizontalDiff.Small");
}

void SHTNDiff::ResetGraphEditors()
{
	if (const TSharedPtr<SGraphEditor> OldGraphEditor = PanelOld.GraphEditor.Pin())
	{
		if (const TSharedPtr<SGraphEditor> NewGraphEditor = PanelNew.GraphEditor.Pin())
		{
			if (bLockViews)
			{
				OldGraphEditor->LockToGraphEditor(NewGraphEditor);
				NewGraphEditor->LockToGraphEditor(OldGraphEditor);
			}
			else
			{
				OldGraphEditor->UnlockFromGraphEditor(NewGraphEditor);
				NewGraphEditor->UnlockFromGraphEditor(OldGraphEditor);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// FHTNDiffPanel
//////////////////////////////////////////////////////////////////////////

SHTNDiff::FHTNDiffPanel::FHTNDiffPanel() :
	HTN(nullptr),
	bShowAssetName(false)
{}

TSharedRef<SWidget> SHTNDiff::FHTNDiffPanel::CreateGraphWidget(
#if UE_VERSION_OLDER_THAN(5, 1, 0)
	UEdGraph* const GraphToDiff)
#else
	TSharedPtr<TArray<FDiffSingleResult>> DiffResults)
#endif
{
	UEdGraph* const Graph = HTN ? HTN->HTNGraph : nullptr;

	// Make graph editor widget
	if (!Graph)
	{
		return SNew(SBorder)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(LOCTEXT("HTNDifPanelNoGraphTip", "Graph does not exist in this revision"))
		];
	}

	if (!GraphEditorCommands.IsValid())
	{
		GraphEditorCommands = MakeShareable(new FUICommandList());
		GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
			FExecuteAction::CreateRaw(this, &FHTNDiffPanel::CopySelectedNodes),
			FCanExecuteAction::CreateRaw(this, &FHTNDiffPanel::CanCopyNodes));
	}

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateRaw(this, &SHTNDiff::FHTNDiffPanel::OnSelectionChanged);

	return SNew(SBorder)
	.VAlign(VAlign_Fill)
	[
		SNew(SOverlay)
		// Graph slot
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(GraphEditor, SGraphEditor)
				.AdditionalCommands(GraphEditorCommands)
				.GraphToEdit(Graph)
#if UE_VERSION_OLDER_THAN(5, 1, 0)
				.GraphToDiff(GraphToDiff)
#else
				.DiffResults(DiffResults)
#endif
				.IsEditable(false)
				// Hide the thick white border and the huge "READ-ONLY" text overlay
				.ShowGraphStateOverlay(false)
				.GraphEvents(InEvents)
			]
		]
		// Revision info slot
		+ SOverlay::Slot()
		.VAlign(VAlign_Bottom)
		.HAlign(HAlign_Right)
		.Padding(FMargin(20.0f, 10.0f))
		[
			SNew(SBox)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.TextStyle(FHTNAppStyle::Get(), "DetailsView.CategoryTextStyle")
				.Text(GetRevisionDescription())
				.ShadowColorAndOpacity(FColor::Black)
				.ShadowOffset(FVector2D(1.4, 1.4))
			]
		]
	];
}

TSharedRef<SWidget> SHTNDiff::FHTNDiffPanel::CreateDetailsWidget()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(nullptr);
	DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateRaw(this, &SHTNDiff::FHTNDiffPanel::IsPropertyEditable));

	return SNew(SBorder)
	.Visibility(EVisibility::Visible)
	.BorderImage(FHTNAppStyle::GetBrush("Docking.Tab", ".ContentAreaBrush"))
	[
		DetailsView.ToSharedRef()
	];
}

FText SHTNDiff::FHTNDiffPanel::GetRevisionDescription() const
{
	FText Result = LOCTEXT("CurrentRevision", "Current Revision");

	const FText AssetNameText = FText::FromString(GetNameSafe(HTN));

	// if this isn't the current working version being displayed
	if (!RevisionInfo.Revision.IsEmpty())
	{
		// Don't use grouping on the revision or CL numbers to match how Perforce displays them
		const FText DateText = FText::AsDate(RevisionInfo.Date, EDateTimeStyle::Short);
		const FText RevisionText = FText::FromString(RevisionInfo.Revision);
		const FText ChangelistText = FText::AsNumber(RevisionInfo.Changelist, &FNumberFormattingOptions::DefaultNoGrouping());

		if (bShowAssetName)
		{
			if (ISourceControlModule::Get().GetProvider().UsesChangelists())
			{
				Result = FText::Format(LOCTEXT("NamedRevisionDiffFmtUsesChangelists", "{0} - Revision {1}, CL {2}, {3}"),
					AssetNameText, RevisionText, ChangelistText, DateText);
			}
			else
			{
				Result = FText::Format(LOCTEXT("NamedRevisionDiffFmt", "{0} - Revision {1}, {2}"),
					AssetNameText, RevisionText, DateText);
			}
		}
		else
		{
			if (ISourceControlModule::Get().GetProvider().UsesChangelists())
			{
				Result = FText::Format(LOCTEXT("PreviousRevisionDifFmtUsesChangelists", "Revision {0}, CL {1}, {2}"),
					RevisionText, ChangelistText, DateText);
			}
			else
			{
				Result = FText::Format(LOCTEXT("PreviousRevisionDifFmt", "Revision {0}, {1}"),
					RevisionText, DateText);
			}
		}
	}
	else if (bShowAssetName)
	{
		Result = FText::Format(LOCTEXT("NamedCurrentRevisionFmt", "{0} - Current Revision"),
			AssetNameText);
	}

	return Result;
}

FGraphPanelSelectionSet SHTNDiff::FHTNDiffPanel::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = GraphEditor.Pin();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}
	return CurrentSelection;
}

void SHTNDiff::FHTNDiffPanel::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool SHTNDiff::FHTNDiffPanel::CanCopyNodes() const
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}
	return false;
}

void SHTNDiff::FHTNDiffPanel::OnSelectionChanged( const FGraphPanelSelectionSet& NewSelection )
{
	const TArray<UObject*> Selection = FHTNEditorUtils::GetSelectionForPropertyEditor(NewSelection);

	if (Selection.Num() == 1)
	{
		if (DetailsView.IsValid())
		{
			DetailsView->SetObjects(Selection);
		}
	}
	else if (DetailsView.IsValid())
	{
		DetailsView->SetObject(nullptr);
	}
}

bool SHTNDiff::FHTNDiffPanel::IsPropertyEditable()
{
	return false;
}

#undef LOCTEXT_NAMESPACE
