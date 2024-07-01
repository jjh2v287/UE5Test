// Copyright 2023-2024, Le Binh Son, All rights reserved.


#include "SResidualDataWidget.h"
#include "Editor.h"

void SResidualDataWidget::Construct(const FArguments& InArgs)
{
	ResidualData = InArgs._ResidualData;
	
	OnResidualDataChanged = InArgs._OnReisdualDataChanged;
	
}

int32 SResidualDataWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// const int32 NumPoints = 512;
	// TArray<FVector2D> Points;
	// Points.Reserve(NumPoints);
	// const FTransform2D PointsTransform = GetPointsTransform(AllottedGeometry);
	// for (int32 PointIndex = 0; PointIndex < NumPoints; ++PointIndex)
	// {
	// 	const float X = PointIndex / (NumPoints - 1.0);
	// 	const float D = (X - Mean.Get()) / StandardDeviation.Get();
	// 	const float Y = FMath::Exp(-0.5f * D * D);
	// 	Points.Add(PointsTransform.TransformPoint(FVector2D(X, Y)));
	// }
	// FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points);
	return LayerId;
}

FVector2D SResidualDataWidget::ComputeDesiredSize(float) const
{
	return FVector2D(200.0, 200.0);
}

// FTransform2D SResidualDataWidget::GetPointsTransform(const FGeometry& AllottedGeometry) const
// {
// 	const double Margin = 0.05 * AllottedGeometry.GetLocalSize().GetMin();
// 	const FScale2D Scale((AllottedGeometry.GetLocalSize() - 2.0 * Margin) * FVector2D(1.0, -1.0));
// 	const FVector2D Translation(Margin, AllottedGeometry.GetLocalSize().Y - Margin);
// 	return FTransform2D(Scale, Translation);
// }
