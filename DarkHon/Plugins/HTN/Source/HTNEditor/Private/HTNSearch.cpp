// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNSearch.h"
#include "EdGraph/EdGraph.h"
#include "HTNAppStyle.h"
#include "HTNEditor.h"
#include "HTNGraphNode.h"
#include "HTNGraphNode_Decorator.h"
#include "HTNGraphNode_Service.h"

#include "Algo/AnyOf.h"
#include "Algo/Transform.h"
#include "SGraphPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "HTNSearch"


FHTNSearchResult::FHTNSearchResult(UEdGraphNode* InNode, TSharedPtr<FHTNSearchResult> InParent, TOptional<FString> MatchedString) :
	GraphNode(InNode),
	MatchedString(MatchedString),
	Parent(MoveTemp(InParent))
{}

TSharedRef<SWidget> FHTNSearchResult::CreateIcon() const
{
	const FSlateBrush* Brush = nullptr;
	if (GraphNode.IsValid())
	{
		if (Cast<UHTNGraphNode_Service>(GraphNode.Get()))
		{
			Brush = FHTNAppStyle::GetBrush(TEXT("GraphEditor.PinIcon"));
		}
		else if (Cast<UHTNGraphNode_Decorator>(GraphNode.Get()))
		{
			Brush = FHTNAppStyle::GetBrush(TEXT("GraphEditor.RefPinIcon"));
		}
		else
		{
			Brush = FHTNAppStyle::GetBrush(TEXT("GraphEditor.FIB_Event"));
		}
	}
	
	return SNew(SImage)
		.Image(Brush)
		.ColorAndOpacity(FSlateColor::UseForeground());
}

FText FHTNSearchResult::GetText() const
{
	if (GraphNode.IsValid())
	{
		return GraphNode->GetNodeTitle(ENodeTitleType::ListView);
	}

	if (GraphNode.IsExplicitlyNull())
	{
		return LOCTEXT("NoResults", "No Results found");
	}

	return LOCTEXT("MissingNodeInResult", "[Missing Node]");
}

FText FHTNSearchResult::GetHighlightText() const
{
	return MatchedString ? FText::FromString(*MatchedString) : FText::GetEmpty();
}

UEdGraphNode* FHTNSearchResult::GetNodeToJumpToOnClick() const
{
	if (UEdGraphNode* const OwnResult = GraphNode.Get())
	{
		return OwnResult;
	}

	if (const TSharedPtr<FHTNSearchResult> ParentPinned = Parent.Pin())
	{
		if (UEdGraphNode* const ParentResult = ParentPinned->GetNodeToJumpToOnClick())
		{
			return ParentResult;
		}
	}

	return nullptr;
}

FHTNSearchContext::FHTNSearchContext(const FString& SearchString)
{
	SearchString.ParseIntoArray(Queries, TEXT("|"), /*InCullEmpty=*/true);
	for (FString& Query : Queries)
	{
		Query.TrimStartAndEndInline();
	}
	Queries.RemoveAll(&FString::IsEmpty);
}

TSharedPtr<FHTNSearchResult> FHTNSearchContext::SearchNode(class UEdGraphNode* Node) const
{
	if (!Node)
	{
		return nullptr;
	}

	if (Queries.IsEmpty())
	{
		return nullptr;
	}

	const TSharedRef<FHTNSearchResult> NodeResult = MakeShared<FHTNSearchResult>(Node, nullptr, Matches(Node));

	// Searching through subnodes
	if (const UHTNGraphNode* const HTNNode = Cast<UHTNGraphNode>(Node))
	{
		for (UHTNGraphNode_Decorator* const Decorator : HTNNode->Decorators)
		{
			if (const TOptional<FString> MatchedDecorator = Matches(Decorator))
			{
				NodeResult->Children.Add(MakeShared<FHTNSearchResult>(Decorator, NodeResult, MatchedDecorator));
			}
		}

		for (UHTNGraphNode_Service* const Service : HTNNode->Services)
		{
			if (const TOptional<FString> MatchedService = Matches(Service))
			{
				NodeResult->Children.Add(MakeShared<FHTNSearchResult>(Service, NodeResult, MatchedService));
			}
		}
	}

	if (NodeResult->MatchedString || !NodeResult->Children.IsEmpty())
	{
		return NodeResult;
	}

	return nullptr;
}

TOptional<FString> FHTNSearchContext::Matches(const UEdGraphNode* Node) const
{
	if (!Node)
	{
		return {};
	}

	const FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
	if (const TOptional<FString> MatchedTitle = Matches(NodeTitle))
	{
		return MatchedTitle;
	}

	if (const TOptional<FString> MatchedComment = Matches(Node->NodeComment))
	{
		return MatchedComment;
	}
	
	if (const UAIGraphNode* const AIGraphNode = Cast<UAIGraphNode>(Node))
	{
		if (const TOptional<FString> MatchedDescription = Matches(AIGraphNode->GetDescription().ToString()))
		{
			return MatchedDescription;
		}
	}

	return {};
}

TOptional<FString> FHTNSearchContext::Matches(const FString& String) const
{
	for (const FString& Query : Queries)
	{
		if (String.Contains(Query))
		{
			return Query;
		}
	}

	return {};
}

void SHTNSearch::Construct(const FArguments& InArgs, TSharedPtr<FHTNEditor> InHTNEditor)
{
	HTNEditorPtr = InHTNEditor;

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SAssignNew(SearchTextField, SSearchBox)
				.HintText(LOCTEXT("HTNSearchHint", "Enter text to find nodes..."))
				.OnTextChanged(this, &SHTNSearch::OnSearchTextChanged)
				.OnTextCommitted(this, &SHTNSearch::OnSearchTextCommitted)
			]
		]
		+SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0.f, 4.f, 0.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(FHTNAppStyle::GetBrush("Menu.Background"))
			[
				SAssignNew(TreeView, STreeViewType)
				.TreeItemsSource(&TopLevelResults)
				.OnGenerateRow(this, &SHTNSearch::OnGenerateRow)
				.OnGetChildren(this, &SHTNSearch::OnGetChildren)
				.OnSelectionChanged(this, &SHTNSearch::OnTreeSelectionChanged)
				.SelectionMode(ESelectionMode::Multi)
			]
		]
	];
}

void SHTNSearch::FocusForUse()
{
	// NOTE: Careful, GeneratePathToWidget can be reentrant in that it can call visibility delegates and such
	FWidgetPath FilterTextBoxWidgetPath;
	FSlateApplication::Get().GeneratePathToWidgetUnchecked(SearchTextField.ToSharedRef(), FilterTextBoxWidgetPath);

	// Set keyboard focus directly
	FSlateApplication::Get().SetKeyboardFocus(FilterTextBoxWidgetPath, EFocusCause::SetDirectly);
}

void SHTNSearch::OnSearchTextChanged(const FText& SearchText)
{	
	UpdateSearch(SearchText);
}

void SHTNSearch::OnSearchTextCommitted(const FText& SearchText, ETextCommit::Type CommitType)
{
	UpdateSearch(SearchText);
}

void SHTNSearch::UpdateSearch(const FText& SearchText)
{
	SetNodeHighlighting(TopLevelResults, false);
	TopLevelResults.Empty();

	// Perform search
	FHTNSearchContext SearchContext(SearchText.ToString());
	if (const UEdGraph* const Graph = GetGraph())
	{
		for (UEdGraphNode* const Node : Graph->Nodes)
		{
			if (const TSharedPtr<FHTNSearchResult> NodeResult = SearchContext.SearchNode(Node))
			{
				TopLevelResults.Add(NodeResult);
			}
		}
	}

	// Insert a fake result to inform the user if nothing was found
	if (TopLevelResults.IsEmpty())
	{
		TopLevelResults.Add(MakeShared<FHTNSearchResult>());
	}

	// Update tree view with results
	TreeView->RequestTreeRefresh();
	for (const TSharedPtr<FHTNSearchResult>& Result : TopLevelResults)
	{
		TreeView->SetItemExpansion(Result, true);
	}
	SetNodeHighlighting(TopLevelResults, true);
}

void SHTNSearch::SetNodeHighlighting(TArrayView<const TSharedPtr<FHTNSearchResult>> Results, bool bHighlight)
{
	for (const TSharedPtr<FHTNSearchResult>& Result : Results)
	{
		if (Result.IsValid())
		{
			if (UHTNGraphNode* const HTNNode = Cast<UHTNGraphNode>(Result->GraphNode.Get()))
			{
				HTNNode->bHighlightDueToSearch = bHighlight && Result->MatchedString;
				HTNNode->SearchHighlightText = HTNNode->bHighlightDueToSearch ? Result->GetHighlightText() : FText::GetEmpty();
			}
			SetNodeHighlighting(Result->Children, bHighlight);
		}
	}
}

TSharedRef<ITableRow> SHTNSearch::OnGenerateRow(TSharedPtr<FHTNSearchResult> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(Item.IsValid());
	return SNew(STableRow<TSharedPtr<FHTNSearchResult>>, OwnerTable)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(SBox)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					Item->CreateIcon()
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(2, 0)
				[
					SNew(STextBlock)
					.Text(Item->GetText())
					.HighlightText(Item->GetHighlightText())
				]
			]
		]
	];
}

void SHTNSearch::OnGetChildren(TSharedPtr<FHTNSearchResult> Item, TArray<TSharedPtr<FHTNSearchResult>>& OutChildren)
{
	if (Item.IsValid())
	{
		OutChildren += Item->Children;
	}
}

void SHTNSearch::OnTreeSelectionChanged(TSharedPtr<FHTNSearchResult> Item, ESelectInfo::Type)
{
	const TSharedPtr<FHTNEditor> Editor = HTNEditorPtr.Pin();
	const TSharedPtr<SGraphEditor> GraphEditor = Editor ? Editor->GetFocusedGraphPtr().Pin() : nullptr;
	if (!GraphEditor)
	{
		return;
	}

	TArray<UEdGraphNode*, TInlineAllocator<32>> NodesToSelect;
	for (const TSharedPtr<FHTNSearchResult>& Result : TreeView->GetSelectedItems())
	{
		if (Result.IsValid())
		{
			if (UEdGraphNode* const NodeToSelect = Result->GetNodeToJumpToOnClick())
			{
				NodesToSelect.AddUnique(NodeToSelect);
			}
		}
	}

	if (!NodesToSelect.IsEmpty())
	{
		// Make the Details tab have the nodes from the selected results.
		GraphEditor->ClearSelectionSet();
		for (UEdGraphNode* const NodeToSelect : NodesToSelect)
		{
			GraphEditor->SetNodeSelection(NodeToSelect, /*bSelect=*/true);
		}

		ZoomToFitNodes(*GraphEditor, NodesToSelect);
	}
}

UEdGraph* SHTNSearch::GetGraph() const
{
	if (const TSharedPtr<FHTNEditor> Editor = HTNEditorPtr.Pin())
	{
		if (const TSharedPtr<SGraphEditor> EditorWidget = Editor->GetFocusedGraphPtr().Pin())
		{
			return EditorWidget->GetCurrentGraph();
		}
	}

	return nullptr;
}

void SHTNSearch::ZoomToFitNodes(const SGraphEditor& GraphEditor, TArrayView<UEdGraphNode* const> Nodes) const
{
	// We can't just use GraphEditor->ZoomToFit(/*bOnlySelection=*/true) 
	// because that can't figure out the bounds of sub-nodes.
	// So instead we manually calculate the bounds and zoom to that.

	SGraphPanel* const GraphPanel = GraphEditor.GetGraphPanel();
	if (!GraphPanel)
	{
		return;
	}

	TArray<UEdGraphNode*, TInlineAllocator<32>> TopLevelNodes;
	Algo::Transform(Nodes, TopLevelNodes, [](UEdGraphNode* Node) -> UEdGraphNode*
	{
		const UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(Node);
		if (HTNGraphNode && HTNGraphNode->bIsSubNode && HTNGraphNode->ParentNode)
		{
			return HTNGraphNode->ParentNode;
		}

		return Node;
	});

#if UE_VERSION_OLDER_THAN(5, 6, 0)
	using FSlatePos = FVector2D;
#else
	using FSlatePos = FVector2f;
#endif

	FSlatePos MinCorner(FLT_MAX, FLT_MAX);
	FSlatePos MaxCorner(-FLT_MAX, -FLT_MAX);
	for (UEdGraphNode* const Node : TopLevelNodes)
	{
		FSlatePos MinCornerOfNode(FLT_MAX, FLT_MAX);
		FSlatePos MaxCornerOfNode(-FLT_MAX, -FLT_MAX);
		if (GraphPanel->GetBoundsForNode(Node, MinCornerOfNode, MaxCornerOfNode))
		{
			MinCorner = FSlatePos(FMath::Min(MinCorner.X, MinCornerOfNode.X), FMath::Min(MinCorner.Y, MinCornerOfNode.Y));
			MaxCorner = FSlatePos(FMath::Max(MaxCorner.X, MaxCornerOfNode.X), FMath::Max(MaxCorner.Y, MaxCornerOfNode.Y));
		}
	}

	// ZoomToTarget is protected so we do this hack to access it.
	struct FProtectedGetter : public SGraphPanel { using SGraphPanel::ZoomToTarget; };
	StaticCast<FProtectedGetter*>(GraphPanel)->ZoomToTarget(MinCorner, MaxCorner);
}

#undef LOCTEXT_NAMESPACE
