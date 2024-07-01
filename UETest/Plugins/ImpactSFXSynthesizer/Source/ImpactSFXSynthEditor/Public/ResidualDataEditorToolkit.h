// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CurveEditor.h"
#include "EditorUndoClient.h"
#include "ImpactSynthCurveEditorModel.h"
#include "ResidualData.h"
#include "Toolkits/AssetEditorToolkit.h"

namespace LBSImpactSFXSynth
{
	class FResidualSynth;
}

class FResidualSynth;

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		class IMPACTSFXSYNTHEDITOR_API FResidualDataEditorToolkit : public FAssetEditorToolkit, public FNotifyHook, public FEditorUndoClient
		{
		public:
			void Init(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InParentObject);

			/** FAssetEditorToolkit interface */
			virtual FName GetToolkitFName() const override { return TEXT("ResidualDataEditor"); }
			virtual FText GetBaseToolkitName() const override { return INVTEXT("Residual Data Editor"); }
			virtual FString GetWorldCentricTabPrefix() const override { return TEXT("Residual Data "); }
			virtual FLinearColor GetWorldCentricTabColorScale() const override { return {}; }
			virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
			virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;

			/** FNotifyHook interface */
			virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;

			virtual bool IsCurvePropertyChanged(FProperty* PropertyThatChanged);
			
			/** Updates & redraws curves. */
			void RefreshCurves();

			static bool SynthesizePreviewedSound(UResidualData* ResidualData, Audio::FAlignedFloatBuffer& SynthesizedData);
			/**
			 * @brief Play synthesized data in editor. WARNING: Data must be generated using current device playback rate
			 * @param InData Synthesized Data in device playback sample rate
			 */
			static bool PlaySynthesizedSound(TArrayView<const float> InData);

		protected:
			struct FCurveData
			{
				FCurveModelID ModelID;

				/* Curve used purely for display.  May be down-sampled from
				 * asset's curve for performance while editing */
				TSharedPtr<FRichCurve> ExpressionCurve;

				FCurveData()
					: ModelID(FCurveModelID::Unique())
				{
				}
			};
			
			virtual void PostUndo(bool bSuccess) override;
			virtual void PostRedo(bool bSuccess) override;

			void SetCurve(int32 InIndex, FRichCurve& InRichCurve, EImpactSynthCurveSource InSource);
			FImpactSynthCurve* GetImpactSynthCurve(int32 InIndex);
			
			virtual TUniquePtr<FImpactSynthCurveModel> ConstructCurveModel(FRichCurve& InRichCurve, UObject* InParentObject, const FImpactSynthCurve* InSynthCurve);
			
		private:
			TSharedPtr<FCurveEditor> CurveEditor;
			TSharedPtr<SCurveEditorPanel> CurvePanel;
			
			TArray<FCurveData> CurveData;

			/** Properties tab */
			TSharedPtr<IDetailsView> PropertiesView;

			FImpactSynthCurve SynthesizedCurve;
			FImpactSynthCurve MagFreqCurve;
			
			static const FName AppIdentifier;
			static const FName PropertiesTabId;
			static const FName CurveTabId;

			void InitCurves();
			void ResetCurves();

			int32 GetNumCurve() const;
			
			/**	Spawns the tab allowing for editing/viewing the output curve(s) */
			TSharedRef<SDockTab> SpawnTab_OutputCurve(const FSpawnTabArgs& Args);

			/**	Spawns the tab allowing for editing/viewing the output curve(s) */
			TSharedRef<SDockTab> SpawnTab_Properties(const FSpawnTabArgs& Args);

			void SetImpactSynthCurve(const int32 Index, FImpactSynthCurve& ImpactSynthCurve);
			
			/** Clears the expression curve at the given input index */
			void ClearExpressionCurve(int32 InIndex);

			void GenerateDefaultCurve(FCurveData& OutCurveData, const int32 Index, FImpactSynthCurve& ImpactSynthCurve, const EImpactSynthCurveSource Source);

			static void TrimKeys(FImpactSynthCurve& ImpactSynthCurve);

			bool RequiresNewCurve(int32 InIndex, const FRichCurve& InRichCurve) const;

			void OnResidualDataSynthesized();
			void SynthesizeDataAndCurves(Audio::FAlignedFloatBuffer& SynthesizedData);
			void AddMagFreqCurveKeys(const LBSImpactSFXSynth::FResidualSynth& ResidualSynth, TArrayView<const float> MagFreq);

			void OnStopPlayingResidualData();
			
			void OnResidualDataError(const FString& InTitle, const FString& InMessage);
		};
	}
}