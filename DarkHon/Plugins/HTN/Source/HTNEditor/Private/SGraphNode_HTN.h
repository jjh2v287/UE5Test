// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SGraphPin.h"
#include "SGraphNodeAI.h"

// The widget that displays an UHTNGraphNode in the graph editor.
class SGraphNode_HTN : public SGraphNodeAI
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_HTN) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UHTNGraphNode* InNode);

	// SGraphNode interface
	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;
	virtual void GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const override;
	virtual TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const override;
	virtual TSharedRef<SGraphNode> GetNodeUnderMouse(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	// End of SGraphNode interface

protected:
	virtual const FSlateBrush* GetNameIcon() const override;
	
	TSharedRef<SWidget> CreateNodeBody();
	void AddDecorator(class UHTNGraphNode_Decorator* DecoratorGraphNode);
	void AddService(class UHTNGraphNode_Service* ServiceGraphNode);
	void AddCommentBubble();

	FSlateColor GetBorderBackgroundColor() const;
	FSlateColor GetBackgroundColor() const;
	FText GetSearchHighlightText() const;

	TSharedPtr<SBorder> NodeBody;
	
	TSharedPtr<SVerticalBox> DecoratorsBox;
	TArray<TSharedPtr<SGraphNode>> DecoratorWidgets;

	TSharedPtr<SVerticalBox> ServicesBox;
	TArray<TSharedPtr<SGraphNode>> ServiceWidgets;
};