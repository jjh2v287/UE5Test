// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "SGraphNode_HTN.h"
#include "IDocumentation.h"
#include "NodeFactory.h"
#include "GraphEditorSettings.h"
#include "SCommentBubble.h"
#include "SGraphPanel.h"
#include "SGraphPreviewer.h"
#include "SLevelOfDetailBranchNode.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#include "HTNAppStyle.h"
#include "HTNColors.h"
#include "HTNEditor.h"
#include "HTNGraphNode.h"
#include "HTNGraphNode_Root.h"
#include "HTNGraphNode_Decorator.h"
#include "HTNGraphNode_Service.h"
#include "HTNTask.h"
#include "HTNStyle.h"
#include "Tasks/HTNTask_SubPlan.h"
#include "Nodes/HTNNode_SubNetwork.h"
#include "Nodes/HTNNode_SubNetworkDynamic.h"

#define LOCTEXT_NAMESPACE "HTNEditor"

namespace
{
	class SHTNPin : public SGraphPinAI
	{
	public:
		SLATE_BEGIN_ARGS(SHTNPin) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, UEdGraphPin* InPin) { SGraphPinAI::Construct({}, InPin); }
		
	protected:
		
		virtual FSlateColor GetPinColor() const override
		{
#if UE_VERSION_OLDER_THAN(5, 1, 0)
			const bool bDiffing = GraphPinObj->bIsDiffing;
#else
			const bool bDiffing = bIsDiffHighlighted;
#endif
			return
				bDiffing ? HTNColors::Pin::Diff :
				IsHovered() ? HTNColors::Pin::Hover :
				HTNColors::Pin::Default;
		}
	};
}

void SGraphNode_HTN::Construct(const FArguments& InArgs, UHTNGraphNode* InNode)
{
	SGraphNodeAI::Construct({}, InNode);
}

void SGraphNode_HTN::UpdateGraphNode()
{
	bDragMarkerVisible = false;
	InputPins.Empty();
	OutputPins.Empty();
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	SubNodes.Reset();

	// Update decorator & service widgets
	{
		DecoratorWidgets.Reset();
		if (DecoratorsBox.IsValid())
		{
			DecoratorsBox->ClearChildren();
		}
		else
		{
			SAssignNew(DecoratorsBox, SVerticalBox);
		}

		ServiceWidgets.Reset();
		if (ServicesBox.IsValid())
		{
			ServicesBox->ClearChildren();
		}
		else
		{
			SAssignNew(ServicesBox, SVerticalBox);
		}

		if (UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode))
		{
			for (UHTNGraphNode_Decorator* const DecoratorNode : HTNGraphNode->Decorators)
			{
				AddDecorator(DecoratorNode);
			}

			for (UHTNGraphNode_Service* const ServiceNode : HTNGraphNode->Services)
			{
				AddService(ServiceNode);
			}
		}
	}

	const FMargin NodePadding = Cast<UHTNGraphNode_Decorator>(GraphNode) || Cast<UHTNGraphNode_Service>(GraphNode) ? 
		FMargin(2.0f) :
		FMargin(8.0f);

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FHTNAppStyle::GetBrush("Graph.StateNode.Body"))
			.Padding(0.0f)
			.BorderBackgroundColor(this, &SGraphNode_HTN::GetBorderBackgroundColor)
			.OnMouseButtonDown(this, &SGraphNode_HTN::OnMouseDown)
			[
				SNew(SOverlay)

				// Pins and node details
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SHorizontalBox)

					// Input pins
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f)
					[
						SNew(SBox)
						.MinDesiredWidth(NodePadding.Left)
						[
							SAssignNew(LeftNodeBox, SVerticalBox)
						]
					]

					// Node body
					+ SHorizontalBox::Slot()
					.Padding(FMargin(0.0f, NodePadding.Top, 0.0f, NodePadding.Bottom))
					[
						SNew(SVerticalBox)

						// Decorators
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							DecoratorsBox.ToSharedRef()
						]

						// Main body
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							CreateNodeBody()
						]

						// Services
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							ServicesBox.ToSharedRef()
						]
					]

					// Output pins
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.MinDesiredWidth(NodePadding.Right)
						[
							SAssignNew(RightNodeBox, SVerticalBox)
						]
					]
				]
			]
		];

	AddCommentBubble();
	CreatePinWidgets();
}

TSharedRef<SWidget> SGraphNode_HTN::CreateNodeBody()
{
	const TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	TWeakPtr<SNodeTitle> WeakNodeTitle = NodeTitle;
	const auto GetNodeTitlePlaceholderWidth = [WeakNodeTitle]() -> FOptionalSize {
		const TSharedPtr<SNodeTitle> NodeTitlePin = WeakNodeTitle.Pin();
		const float DesiredWidth = NodeTitlePin.IsValid() ? NodeTitlePin->GetTitleSize().X : 0.0f;
		return FMath::Max(75.0f, DesiredWidth);
	};
	const auto GetNodeTitlePlaceholderHeight = [WeakNodeTitle]() -> FOptionalSize {
		const TSharedPtr<SNodeTitle> NodeTitlePin = WeakNodeTitle.Pin();
		const float DesiredHeight = NodeTitlePin.IsValid() ? NodeTitlePin->GetTitleSize().Y : 0.0f;
		return FMath::Max(22.0f, DesiredHeight);
	};
	
	return SAssignNew(NodeBody, SBorder)
	.BorderImage(FHTNAppStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
	.BorderBackgroundColor(HTNColors::NodeBody::Root)
	.BorderBackgroundColor(this, &SGraphNode_HTN::GetBackgroundColor)
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Center)
	.Visibility(EVisibility::SelfHitTestInvisible)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(4.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SLevelOfDetailBranchNode)
				.UseLowDetailSlot(this, &SGraphNode_HTN::UseLowDetailNodeTitles)
				.LowDetail()
				[
					SNew(SBox)
					.WidthOverride_Lambda(GetNodeTitlePlaceholderWidth)
					.HeightOverride_Lambda(GetNodeTitlePlaceholderHeight)
				]
				.HighDetail()
				[
					SNew(SHorizontalBox)
					
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(this, &SGraphNode_HTN::GetNameIcon)
					]

					+ SHorizontalBox::Slot()
					.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(InlineEditableText, SInlineEditableTextBlock)
							.Style(FHTNAppStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
							.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
							.HighlightText(this, &SGraphNode_HTN::GetSearchHighlightText)
							.OnVerifyTextChanged(this, &SGraphNode_HTN::OnVerifyNameTextChanged)
							.OnTextCommitted(this, &SGraphNode_HTN::OnNameTextCommited)
							.IsReadOnly(this, &SGraphNode_HTN::IsNameReadOnly)
							.IsSelected(this, &SGraphNode_HTN::IsSelectedExclusively)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							NodeTitle.ToSharedRef()
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				// DESCRIPTION MESSAGE
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Example node description.")))
				.Visibility(this, &SGraphNode_HTN::GetDescriptionVisibility)
				.Text(this, &SGraphNode_HTN::GetDescription)
				.HighlightText(this, &SGraphNode_HTN::GetSearchHighlightText)
			]
		]
	];
}

void SGraphNode_HTN::CreatePinWidgets()
{
	for (UEdGraphPin* const Pin : GraphNode->Pins)
	{
		if (Pin && !Pin->bHidden)
		{
			AddPin(SNew(SHTNPin, Pin));
		}
	}
}

void SGraphNode_HTN::AddPin(const TSharedRef<SGraphPin>& PinWidget)
{
	PinWidget->SetOwner(SharedThis(this));

	const auto AddPinTo = [&](SVerticalBox& Box)
	{
		Box.AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			.Padding(0.0f, 4.0f)
			[
				SNew(SBox)
				.MinDesiredHeight(40.f)
				[
					PinWidget
				]
			];
	};
	
	if (PinWidget->GetDirection() == EGPD_Input)
	{
		AddPinTo(*LeftNodeBox);
		InputPins.Add(PinWidget);
	}
	else // Direction == EGPD_Output
	{
		AddPinTo(*RightNodeBox);
		OutputPins.Add(PinWidget);
	}
}

TSharedPtr<SToolTip> SGraphNode_HTN::GetComplexTooltip()
{
	if (UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode))
	{
		if (UHTNNode_SubNetwork* const SubNetworkNode = Cast<UHTNNode_SubNetwork>(HTNGraphNode->NodeInstance))
		{
			if (SubNetworkNode && SubNetworkNode->HTN && SubNetworkNode->HTN->HTNGraph)
			{
				return SNew(SToolTip)
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					[
						// Create the tooltip graph preview, make sure to disable state overlays to
						// prevent the PIE / read-only borders from obscuring the graph
						SNew(SGraphPreviewer, SubNetworkNode->HTN->HTNGraph)
						.CornerOverlayText(LOCTEXT("HTNSubNetworkNodeOverlayText", "Sub HTN"))
						.ShowGraphStateOverlay(false)
					]
					+SOverlay::Slot()
					.Padding(2.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("HTNSubNetworkNodeTooltip", "Double-click to Open"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				];
			}
		}
	}

	return IDocumentation::Get()->CreateToolTip(
		TAttribute<FText>(this, &SGraphNode::GetNodeTooltip), /*OverrideContent=*/nullptr, 
		GraphNode->GetDocumentationLink(), GraphNode->GetDocumentationExcerptName()
	);
}

void SGraphNode_HTN::GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const
{
	UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode);
	if (!HTNGraphNode)
	{
		return;
	}

	if (HTNGraphNode->bHasBreakpoint)
	{
		FOverlayBrushInfo BreakpointOverlayInfo;
		BreakpointOverlayInfo.Brush = HTNGraphNode->bIsBreakpointEnabled ?
			FHTNAppStyle::GetBrush(TEXT("BTEditor.DebuggerOverlay.Breakpoint.Enabled")) :
			FHTNAppStyle::GetBrush(TEXT("BTEditor.DebuggerOverlay.Breakpoint.Disabled"));

		if (BreakpointOverlayInfo.Brush)
		{
			BreakpointOverlayInfo.OverlayOffset -= BreakpointOverlayInfo.Brush->ImageSize * 0.5f;
		}

		Brushes.Add(BreakpointOverlayInfo);
	}
}

TArray<FOverlayWidgetInfo> SGraphNode_HTN::GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const
{
	TArray<FOverlayWidgetInfo> Widgets;

	check(NodeBody.IsValid());

	FVector2D Origin(0.0f, 0.0f);

	// build overlays for decorator sub-nodes
	for (const auto& DecoratorWidget : DecoratorWidgets)
	{
		TArray<FOverlayWidgetInfo> OverlayWidgets = DecoratorWidget->GetOverlayWidgets(bSelected, WidgetSize);
		for (auto& OverlayWidget : OverlayWidgets)
		{
			OverlayWidget.OverlayOffset.Y += Origin.Y;
		}
		Widgets.Append(OverlayWidgets);
		Origin.Y += DecoratorWidget->GetDesiredSize().Y;
	}

	Origin.Y += NodeBody->GetDesiredSize().Y;

	return Widgets;
}

TSharedRef<SGraphNode> SGraphNode_HTN::GetNodeUnderMouse(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, MouseEvent);
	return SubNode.IsValid() ? SubNode.ToSharedRef() : StaticCastSharedRef<SGraphNode>(AsShared());
}

const FSlateBrush* SGraphNode_HTN::GetNameIcon() const
{
	UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode);
	return HTNGraphNode ? FHTNAppStyle::GetBrush(HTNGraphNode->GetIconName()) : FHTNAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Icon"));
}

void SGraphNode_HTN::AddDecorator(UHTNGraphNode_Decorator* DecoratorGraphNode)
{
	if (!DecoratorGraphNode)
	{
		return;
	}
	
	const TSharedPtr<SGraphNode> DecoratorWidget = FNodeFactory::CreateNodeWidget(DecoratorGraphNode);
	if (OwnerGraphPanelPtr.IsValid())
	{
		DecoratorWidget->SetOwner(OwnerGraphPanelPtr.Pin().ToSharedRef());
		OwnerGraphPanelPtr.Pin()->AttachGraphEvents(DecoratorWidget);
	}

	DecoratorsBox->AddSlot().AutoHeight()
	[
		DecoratorWidget.ToSharedRef()
	];
	DecoratorWidgets.Add(DecoratorWidget);
	AddSubNode(DecoratorWidget);
	
	DecoratorWidget->UpdateGraphNode();
}

void SGraphNode_HTN::AddService(UHTNGraphNode_Service* ServiceGraphNode)
{
	if (!ServiceGraphNode)
	{
		return;
	}

	const TSharedPtr<SGraphNode> ServiceWidget = FNodeFactory::CreateNodeWidget(ServiceGraphNode);
	if (OwnerGraphPanelPtr.IsValid())
	{
		ServiceWidget->SetOwner(OwnerGraphPanelPtr.Pin().ToSharedRef());
		OwnerGraphPanelPtr.Pin()->AttachGraphEvents(ServiceWidget);
	}

	ServicesBox->AddSlot().AutoHeight()
	[
		ServiceWidget.ToSharedRef()
	];
	ServiceWidgets.Add(ServiceWidget);
	AddSubNode(ServiceWidget);

	ServiceWidget->UpdateGraphNode();
}

void SGraphNode_HTN::AddCommentBubble()
{
	const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;
	const TSharedRef<SCommentBubble> CommentBubble = SNew(SCommentBubble)
		.GraphNode(GraphNode)
		.Text(this, &SGraphNode::GetNodeComment)
		.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
		.ColorAndOpacity(CommentColor)
		.AllowPinning(true)
		.EnableTitleBarBubble(true)
		.EnableBubbleCtrls(true)
		.GraphLOD(this, &SGraphNode::GetCurrentLOD)
		.IsGraphNodeHovered(this, &SGraphNode::IsHovered);

	GetOrAddSlot(ENodeZone::TopCenter)
		.SlotOffset(TAttribute<FVector2D>(CommentBubble, &SCommentBubble::GetOffset))
		.SlotSize(TAttribute<FVector2D>(CommentBubble, &SCommentBubble::GetSize))
		.AllowScaling(TAttribute<bool>(CommentBubble, &SCommentBubble::IsScalingAllowed))
		.VAlign(VAlign_Top)
		[
			CommentBubble
		];
}

FSlateColor SGraphNode_HTN::GetBorderBackgroundColor() const
{
	if (UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode))
	{
		const bool bHighlightDueToSearch = HTNGraphNode->bHighlightDueToSearch && FHTNEditor::IsPIENotSimulating();
		const bool bIsSubNodeSelected = HTNGraphNode->bIsSubNode && GetOwnerPanel()->SelectionManager.SelectedNodes.Contains(GraphNode);
		if (bHighlightDueToSearch && bIsSubNodeSelected)
		{
			return HTNColors::NodeBorder::SearchHighlightSelected;
		}
		else if (bHighlightDueToSearch)
		{
			return HTNColors::NodeBorder::SearchHighlight;
		}
		else if (bIsSubNodeSelected)
		{
			return HTNColors::NodeBorder::Selected;
		}

		if (HTNGraphNode->bDebuggerMarkCurrentlyActive)
		{
			return HTNColors::NodeBorder::DebugExecuting;
		}

		if (HTNGraphNode->DebuggerPlanEntries.Num())
		{
			return HTNGraphNode->IsInFutureOfDebuggedPlan() ?
				HTNColors::NodeBorder::DebugFuturePartOfPlan :
				HTNColors::NodeBorder::DebugPastPartOfPlan;
		}
				
		const bool bIsConnectedRoot = Cast<UHTNGraphNode_Root>(HTNGraphNode) && HTNGraphNode->Pins.IsValidIndex(0) && HTNGraphNode->Pins[0]->LinkedTo.Num() > 0;
		if (bIsConnectedRoot)
		{
			return HTNColors::NodeBorder::Root;
		}
	}
	
	return HTNColors::NodeBorder::Inactive;
}

FSlateColor SGraphNode_HTN::GetBackgroundColor() const
{
	FLinearColor NodeColor = HTNColors::NodeBody::Default;

	if (UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode))
	{
		if (Cast<UHTNGraphNode_Decorator>(GraphNode))
		{
			NodeColor = HTNColors::NodeBody::Decorator;
		}
		else if (Cast<UHTNGraphNode_Service>(GraphNode))
		{
			NodeColor = HTNColors::NodeBody::Service;
		}
		else if (Cast<UHTNTask>(HTNGraphNode->NodeInstance) && !Cast<UHTNTask_SubPlan>(HTNGraphNode->NodeInstance))
		{
			NodeColor = HTNColors::NodeBody::Task;
		}
		else if (Cast<UHTNNode_SubNetwork>(HTNGraphNode->NodeInstance) || Cast<UHTNNode_SubNetworkDynamic>(HTNGraphNode->NodeInstance))
		{
			NodeColor = HTNColors::NodeBody::TaskSpecial;
		}
		else if (Cast<UHTNGraphNode_Root>(GraphNode))
		{
			const bool bIsConnected = GraphNode->Pins.IsValidIndex(0) && GraphNode->Pins[0]->LinkedTo.Num() > 0;
			if (bIsConnected)
			{
				NodeColor = HTNColors::NodeBody::Root;
			}
		}
	}

	return NodeColor;
}

FText SGraphNode_HTN::GetSearchHighlightText() const
{
	if (UHTNGraphNode* const HTNGraphNode = Cast<UHTNGraphNode>(GraphNode))
	{
		return HTNGraphNode->SearchHighlightText;
	}

	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
