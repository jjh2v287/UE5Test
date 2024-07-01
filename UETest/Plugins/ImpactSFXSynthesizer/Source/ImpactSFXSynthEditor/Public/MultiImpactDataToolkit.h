// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "ImpactSynthCurveEditorModel.h"
#include "MultiImpactData.h"
#include "Toolkits/AssetEditorToolkit.h"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		class IMPACTSFXSYNTHEDITOR_API FMultiImpactDataToolkit : public FAssetEditorToolkit
		{
		public:
			static constexpr int32 NumSamplesPerPowerPoint = 256;
			
			void Init(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InParentObject);

			/** FAssetEditorToolkit interface */
			virtual FName GetToolkitFName() const override { return TEXT("MultiImpactDataEditor"); }
			virtual FText GetBaseToolkitName() const override { return INVTEXT("Multi Impact Data Editor"); }
			virtual FString GetWorldCentricTabPrefix() const override { return TEXT("Multi Impact Data "); }
			virtual FLinearColor GetWorldCentricTabColorScale() const override { return {}; }
			virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
			virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;

			static float SynthesizeImpact(const UMultiImpactData* MultiImpactData, Audio::FAlignedFloatBuffer& SynthesizedData, const FImpactSpawnInfo& SpawnInfo);
			
		protected:
			void RefreshCurves();
			
		private:
			TSharedPtr<FCurveEditor> CurveEditor;
			TSharedPtr<SCurveEditorPanel> CurvePanel;

			FImpactSynthCurve SynthesizedCurve;
			int32 SynthesizedCurveIndex = 0;
			FCurveModelID SynthesizedCurveID = FCurveModelID::Unique();

			/** Properties tab */
			TSharedPtr<IDetailsView> PropertiesView;
			
			static const FName AppIdentifier;
			static const FName PropertiesTabId;
			static const FName CurveTabId;
			
			TSharedRef<SDockTab> SpawnTab_Properties(const FSpawnTabArgs& Args);
			TSharedRef<SDockTab> SpawnTab_OutputCurve(const FSpawnTabArgs& Args);
			
			void OnPlayingVariation(const UMultiImpactData* InData, const FImpactSpawnInfo& SpawnInfo, const bool bIsPlay);
			
			void AddSynthDataToCurve(Audio::FAlignedFloatBuffer& SynthesizedData, const float SamplingRate);
		};
	}
}