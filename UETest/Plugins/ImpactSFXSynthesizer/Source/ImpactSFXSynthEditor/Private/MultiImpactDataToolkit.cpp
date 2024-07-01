// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "MultiImpactDataToolkit.h"

#include "AudioDevice.h"
#include "ImpactModalObj.h"
#include "ModalSynth.h"
#include "MultiImpactData.h"
#include "ResidualDataEditorToolkit.h"
#include "ResidualSynth.h"
#include "SCurveEditorPanel.h"
#include "DSP/FloatArrayMath.h"
#include "Widgets/Docking/SDockTab.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "MultiImpactDataEditor"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		const FName FMultiImpactDataToolkit::AppIdentifier(TEXT("MultiImpactDataEditorApp"));
		const FName FMultiImpactDataToolkit::PropertiesTabId(TEXT("MultiImpactDataEditor_Properties"));
		const FName FMultiImpactDataToolkit::CurveTabId(TEXT("MultiImpactDataEditor_Curves"));
		
		void FMultiImpactDataToolkit::Init(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InParentObject)
		{
			check(InParentObject);

			SynthesizedCurve.CurveSource = EImpactSynthCurveSource::Generated;
			SynthesizedCurve.DisplayName = TEXT("Energy of The Synthesized Signal");
			SynthesizedCurve.DisplayType = EImpactSynthCurveDisplayType::MagnitudeOverTime;
			SynthesizedCurve.CurveShared = nullptr;
			SynthesizedCurve.CurveCustom.Reset();

			CurveEditor = MakeShared<FCurveEditor>();
			FCurveEditorInitParams InitParams;
			CurveEditor->InitCurveEditor(InitParams);
			CurveEditor->GridLineLabelFormatXAttribute = LOCTEXT("GridXLabelFormat", "{0}");

			TUniquePtr<ICurveEditorBounds> EditorBounds = MakeUnique<FStaticCurveEditorBounds>();
			EditorBounds->SetInputBounds(0.05, 1.05);
			CurveEditor->SetBounds(MoveTemp(EditorBounds));
			
			CurvePanel = SNew(SCurveEditorPanel, CurveEditor.ToSharedRef());

			FDetailsViewArgs Args;
			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertiesView = PropertyModule.CreateDetailView(Args);
			PropertiesView->SetObject(InParentObject);

			TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("MultiImpactDataEditor_Layoutv1")
			->AddArea
			(
				FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.95f)
					->SetOrientation(Orient_Horizontal)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.35f)
						->AddTab(PropertiesTabId, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewSplitter()
						->SetSizeCoefficient(0.65f)
						->SetOrientation(Orient_Vertical)
						->Split
						(
							FTabManager::NewStack()
							//->SetHideTabWell(true)
							->SetSizeCoefficient(0.33f)
							->AddTab(CurveTabId, ETabState::OpenedTab)
						)
					)
				)
			);

			const bool bCreateDefaultStandaloneMenu = true;
			const bool bCreateDefaultToolbar = true;
			const bool bToolbarFocusable = false;
			const bool bUseSmallIcons = true;
			FAssetEditorToolkit::InitAssetEditor(
				Mode,
				InitToolkitHost,
				AppIdentifier,
				StandaloneDefaultLayout,
				bCreateDefaultStandaloneMenu,
				bCreateDefaultToolbar,
				InParentObject,
				bToolbarFocusable,
				bUseSmallIcons);
			
			if(UMultiImpactData* MultiImpactData = Cast<UMultiImpactData>(InParentObject))
			{
				MultiImpactData->OnPlayVariationSound.BindRaw(this, &FMultiImpactDataToolkit::OnPlayingVariation);
			}
		}

		void FMultiImpactDataToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
		{
			WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_MultiImpactDataEditor", "Multi Impact Data Editor"));
			FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
			
			InTabManager->RegisterTabSpawner(PropertiesTabId, FOnSpawnTab::CreateSP(this, &FMultiImpactDataToolkit::SpawnTab_Properties))
						.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
						.SetGroup(WorkspaceMenuCategory.ToSharedRef())
						.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "MultiImpactDataEditor.Tabs.Details"));

			FSlateIcon CurveIcon(FAppStyle::GetAppStyleSetName(), "MultiImpactDataEditor.Tabs.Properties");
			InTabManager->RegisterTabSpawner(CurveTabId, FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs& Args)
			{
				return SpawnTab_OutputCurve(Args);
			}))
			.SetDisplayName(LOCTEXT("ModificationCurvesTab", "Modification Curves"))
			.SetGroup(WorkspaceMenuCategory.ToSharedRef())
			.SetIcon(CurveIcon);
		}

		void FMultiImpactDataToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
		{
			FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
			InTabManager->UnregisterTabSpawner(CurveTabId);
			InTabManager->UnregisterTabSpawner(PropertiesTabId);
		}

		TSharedRef<SDockTab> FMultiImpactDataToolkit::SpawnTab_Properties(const FSpawnTabArgs& Args)
		{
			check(Args.GetTabId() == PropertiesTabId);

			return SNew(SDockTab)
				.Label(LOCTEXT("MultiImpactDataDetailsTitle", "Details"))
				[
					PropertiesView.ToSharedRef()
				];
		}

		TSharedRef<SDockTab> FMultiImpactDataToolkit::SpawnTab_OutputCurve(const FSpawnTabArgs& Args)
		{
			RefreshCurves();
			
			TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
				.Label(FText::Format(LOCTEXT("MultiImpactDataCurveEditorTitle", "View Curve: {0}"), FText::FromString(GetEditingObject()->GetName())))
				.TabColorScale(GetTabColorScale())
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(0.0f)
					[
						CurvePanel.ToSharedRef()
					]
				];

			return NewDockTab;
		}

		void FMultiImpactDataToolkit::OnPlayingVariation(const UMultiImpactData* InData, const FImpactSpawnInfo& SpawnInfo, const bool bIsPlay)
		{
			if(!GEditor || !GEditor->CanPlayEditorSound())
				return;
			
			if(!bIsPlay)
			{
				GEditor->ResetPreviewAudioComponent();
				return;
			}

			Audio::FAlignedFloatBuffer SynthesizedData;
			const float SamplingRate = SynthesizeImpact(InData, SynthesizedData, SpawnInfo);
			if(SamplingRate > 0.f)
			{
				AddSynthDataToCurve(SynthesizedData, SamplingRate);
				FResidualDataEditorToolkit::PlaySynthesizedSound(SynthesizedData);
			}
		}

		float FMultiImpactDataToolkit::SynthesizeImpact(const UMultiImpactData* MultiImpactData, Audio::FAlignedFloatBuffer& SynthesizedData, const FImpactSpawnInfo& SpawnInfo)
		{
			SynthesizedData.Empty();
			if(FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
			{
				const int32 NumChannel = FMath::Min(1, AudioDevice->GetMaxChannels());
				if(NumChannel < 1)
					return 0.f;

				const float PlaySampleRate = AudioDevice->GetSampleRate();
				FRandomStream RandomStream = FRandomStream(FMath::Rand());
				const auto ModalObj = MultiImpactData->GetModalObjProxy();

				TArray<float> ModalSynthData;
				if(SpawnInfo.bUseModalSynth && ModalObj.IsValid())
				{
					FModalSynth ModalSynth = FModalSynth(PlaySampleRate,
														 ModalObj,
														 SpawnInfo.ModalStartTime,
														 SpawnInfo.ModalDuration,
														 MultiImpactData->GetNumUsedModals(),
														 SpawnInfo.GetModalAmplitudeScaleRand(RandomStream),
														 SpawnInfo.GetModalDecayScaleRand(RandomStream),
														 SpawnInfo.GetModalPitchScaleRand(RandomStream),
														MultiImpactData->GetPreviewImpactStrength(),
														SpawnInfo.DampingRatio,
														0.f,
														MultiImpactData->IsRandomlyGetModal());
					FModalSynth::SynthesizeFullOneChannel(ModalSynth, ModalObj, ModalSynthData);
				}
			
				const auto ResidualDataProxy = MultiImpactData->GetResidualDataAssetProxy();
				if(SpawnInfo.bUseResidualSynth && ResidualDataProxy.IsValid() && ResidualDataProxy->GetResidualData())
				{
					const int32 HopSize = ResidualDataProxy->GetResidualData()->GetNumFFT() / 2;
					FResidualSynth ResidualSynth = FResidualSynth(PlaySampleRate, HopSize, NumChannel,
																  ResidualDataProxy.ToSharedRef(),
																  SpawnInfo.GetResidualPlaySpeedScaleRand(RandomStream),
																  SpawnInfo.GetResidualAmplitudeScaleRand(RandomStream),
																  SpawnInfo.GetResidualPitchScaleRand(RandomStream),
																  -1,
																  SpawnInfo.ResidualStartTime,
																  SpawnInfo.ResidualDuration,
																  false,
																  MultiImpactData->GetPreviewImpactStrength());

					FResidualSynth::SynthesizeFull(ResidualSynth, SynthesizedData);
				}

				const int32 NumAdd = FMath::Min(ModalSynthData.Num(), SynthesizedData.Num());
				if(NumAdd > 0)
				{
					TArrayView<const float> InValue = TArrayView<const float>(ModalSynthData).Slice(0, NumAdd);
					TArrayView<float> InVAccum = TArrayView<float>(SynthesizedData).Slice(0, NumAdd);
					Audio::ArrayAddInPlace(InValue, InVAccum);
				}

				const int32 NumCopy = ModalSynthData.Num() - SynthesizedData.Num();
				if(NumCopy > 0)
				{
					const int32 StartIdx = SynthesizedData.Num();
					SynthesizedData.AddUninitialized(NumCopy);
					TArrayView<float> Dest = TArrayView<float>(SynthesizedData).Slice(StartIdx, NumCopy);
					TArrayView<float> Source = TArrayView<float>(ModalSynthData).Slice(StartIdx, NumCopy);
					FMemory::Memcpy(Dest.GetData(), Source.GetData(), NumCopy * sizeof(float));
				}
				
				Audio::ArrayRangeClamp(SynthesizedData, -1.f, 1.f);
				return PlaySampleRate;
			}
			
			return 0.f;			
		}

		void FMultiImpactDataToolkit::AddSynthDataToCurve(Audio::FAlignedFloatBuffer& SynthesizedData, const float SamplingRate)
		{
			SynthesizedCurve.CurveCustom.Reset();
			const int32 NumData = SynthesizedData.Num();
			const TArrayView<const float> DataView = TArrayView<const float>(SynthesizedData);
			int32 NumRemain = NumData;
			int32 Start = 0;
			while(NumRemain > 0)
			{
				const int32 NumAdd = FMath::Min(NumRemain, NumSamplesPerPowerPoint);
				const float Magnitude = Audio::ArrayGetMagnitude(DataView.Slice(Start, NumAdd));
				SynthesizedCurve.CurveCustom.AddKey(Start / SamplingRate, Magnitude);
				Start += NumSamplesPerPowerPoint;
				NumRemain -= NumSamplesPerPowerPoint;
			}

			RefreshCurves();
		}

		void FMultiImpactDataToolkit::RefreshCurves()
		{
			check(CurveEditor.IsValid());
			
			if(CurveEditor->GetCurves().Num() < 1)
			{
				TUniquePtr<FImpactSynthCurveModel> NewCurve = MakeUnique<FImpactSynthCurveModel>(SynthesizedCurve.CurveCustom,
																							 GetEditingObject(),
																							 SynthesizedCurve.CurveSource,
																							 SynthesizedCurve.DisplayType);
				NewCurve->Refresh(SynthesizedCurve, SynthesizedCurveIndex);
				SynthesizedCurveID = CurveEditor->AddCurve(MoveTemp(NewCurve));
				CurveEditor->PinCurve(SynthesizedCurveID);	
			}
			else
			{
				const TUniquePtr<FCurveModel>& CurveModelPtr = CurveEditor->GetCurves().FindChecked(SynthesizedCurveID);
				check(CurveModelPtr.Get())
				static_cast<FImpactSynthCurveModel*>(CurveModelPtr.Get())->Refresh(SynthesizedCurve, SynthesizedCurveIndex);	
			}
			
			CurveEditor->ZoomToFitAll();
		}
		
	}
}

#undef LOCTEXT_NAMESPACE