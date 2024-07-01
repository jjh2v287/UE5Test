// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "RichCurveEditorModel.h"
#include "Views/SCurveEditorViewStacked.h"
#include "ImpactSynthCurve.h"

enum class ECurveEditorViewID : uint64;

// #include "ImpactSynthCurveEditorModel.generated.h"

class UCurveFloat;

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		class IMPACTSFXSYNTHEDITOR_API FImpactSynthCurveModel : public FRichCurveEditorModelRaw
		{
		public:
			static ECurveEditorViewID WaveTableViewId;

			FImpactSynthCurveModel(FRichCurve& InRichCurve, UObject* InOwner, EImpactSynthCurveSource InSource, EImpactSynthCurveDisplayType InDisplayType);

			const FText& GetAxesDescriptor() const;
			const UObject* GetParentObject() const;
			EImpactSynthCurveSource GetSource() const;
			EImpactSynthCurveDisplayType GetDisplayType() const { return CurveDisplayType; }
			void Refresh(const FImpactSynthCurve& InCurve, int32 InCurveIndex);

			virtual ECurveEditorViewID GetViewId() const { return WaveTableViewId; }
			virtual bool IsReadOnly() const override;
			virtual FLinearColor GetColor() const override;
			virtual void GetValueRange(double& MinValue, double& MaxValue) const override;
			virtual void GetTimeRange(double& MinTime, double& MaxTime) const override;
			virtual bool IsFitTimeRange() const;
			
			int32 GetCurveIndex() const { return CurveIndex; }
			
		protected:
			virtual void RefreshCurveDescriptorText(const FImpactSynthCurve& InCurve, FText& OutShortDisplayName, FText& OutInputAxisName, FText& OutOutputAxisName);
			virtual FColor GetCurveColor(const FImpactSynthCurve& InCurve) const;
			virtual FText GetPropertyEditorDisabledText() const;

			TWeakObjectPtr<UObject> ParentObject;

		private:
			int32 CurveIndex = INDEX_NONE;

			EImpactSynthCurveSource CurveSource = EImpactSynthCurveSource::None;
			EImpactSynthCurveDisplayType CurveDisplayType = EImpactSynthCurveDisplayType::MagnitudeOverTime;
			FText InputAxisName;
			FText AxesDescriptor;
		};

		class IMPACTSFXSYNTHEDITOR_API SViewStacked : public SCurveEditorViewStacked
		{
		public:
			void Construct(const FArguments& InArgs, TWeakPtr<FCurveEditor> InCurveEditor);

		protected:
			virtual void PaintView(
				const FPaintArgs& Args,
				const FGeometry& AllottedGeometry,
				const FSlateRect& MyCullingRect,
				FSlateWindowElementList& OutDrawElements,
				int32 BaseLayerId, 
				const FWidgetStyle& InWidgetStyle,
				bool bParentEnabled) const override;

			virtual void DrawViewGrids(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 BaseLayerId, ESlateDrawEffect DrawEffects) const override;
			virtual void DrawLabels(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 BaseLayerId, ESlateDrawEffect DrawEffects) const override;

			virtual void FormatInputLabels(TArray<FText>& InOutLabels, const FImpactSynthCurveModel& EditorModel, const FNumberFormattingOptions& InLabelFormat) const;
			virtual void FormatOutputLabel(const FImpactSynthCurveModel& EditorModel, const FNumberFormattingOptions& InLabelFormat, FText& InOutLabel) const { }
			
		private:
			struct FGridDrawInfo
			{
				const FGeometry* AllottedGeometry;

				FPaintGeometry PaintGeometry;

				ESlateDrawEffect DrawEffects;
				FNumberFormattingOptions LabelFormat;

				TArray<FVector2D> LinePoints;

				FCurveEditorScreenSpace ScreenSpace;

			private:
				FLinearColor MajorGridColor;
				FLinearColor MinorGridColor;

				int32 BaseLayerId = INDEX_NONE;

				const FCurveModel* CurveModel = nullptr;

				double LowerValue = 0.0;
				double PixelBottom = 0.0;
				double PixelTop = 0.0;

			public:
				FGridDrawInfo(const FGeometry* InAllottedGeometry, const FCurveEditorScreenSpace& InScreenSpace, FLinearColor InGridColor, int32 InBaseLayerId);

				void SetCurveModel(const FCurveModel* InCurveModel);
				const FCurveModel* GetCurveModel() const;
				void SetLowerValue(double InLowerValue);
				int32 GetBaseLayerId() const;
				FLinearColor GetLabelColor() const;
				double GetLowerValue() const;
				FLinearColor GetMajorGridColor() const;
				FLinearColor GetMinorGridColor() const;
				double GetPixelBottom() const;
				double GetPixelTop() const;
			};

			void DrawViewGridLineX(FSlateWindowElementList& OutDrawElements, FGridDrawInfo& DrawInfo, ESlateDrawEffect DrawEffect, double OffsetAlpha, bool bIsMajor) const;
			void DrawViewGridLineY(const float VerticalLine, FSlateWindowElementList& OutDrawElements, FGridDrawInfo &DrawInfo, ESlateDrawEffect DrawEffects, const FText* Label, bool bIsMajor) const;
		};
	} 
} 
