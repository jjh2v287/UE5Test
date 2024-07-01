// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ResidualObj.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_OneParam(FOnReisdualDataChanged, UResidualObj*)

class IMPACTSFXSYNTHEDITOR_API SResidualDataWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SResidualDataWidget)
			: _ResidualData(nullptr)
		{}
		SLATE_ATTRIBUTE(UResidualObj*, ResidualData)
		SLATE_EVENT(FOnReisdualDataChanged, OnReisdualDataChanged)
	SLATE_END_ARGS()
	 
	void Construct(const FArguments& InArgs);
	 
	int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	FVector2D ComputeDesiredSize(float) const override;

private:
	TAttribute<UResidualObj*> ResidualData;
	 
	FOnReisdualDataChanged OnResidualDataChanged;
	 
	// FTransform2D GetPointsTransform(const FGeometry& AllottedGeometry) const;
};
