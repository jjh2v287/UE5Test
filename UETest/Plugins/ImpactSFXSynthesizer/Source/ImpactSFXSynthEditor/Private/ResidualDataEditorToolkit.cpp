// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "ResidualDataEditorToolkit.h"

#include "AudioDevice.h"
#include "EditorDialogLibrary.h"
#include "ImpactSFXSynthEditor.h"
#include "ResidualSynth.h"
#include "SCurveEditorPanel.h"
#include "DSP/FloatArrayMath.h"
#include "ImpactSFXSynth/Public/Utils.h"
#include "Widgets/Docking/SDockTab.h"
#include "Modules/ModuleManager.h"
#include "Sound/SoundWaveProcedural.h"

#define LOCTEXT_NAMESPACE "ResidualDataEditor"

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		const FName FResidualDataEditorToolkit::AppIdentifier(TEXT("ResidualDataEditorApp"));
		const FName FResidualDataEditorToolkit::PropertiesTabId(TEXT("ResidualDataEditor_Properties"));
		const FName FResidualDataEditorToolkit::CurveTabId(TEXT("ResidualDataEditor_Curves"));
		
		void FResidualDataEditorToolkit::Init(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InParentObject)
		{
			check(InParentObject);

			SynthesizedCurve.CurveSource = EImpactSynthCurveSource::Generated;
			SynthesizedCurve.DisplayName = TEXT("Energy of The Synthesized Signal");
			SynthesizedCurve.DisplayType = EImpactSynthCurveDisplayType::MagnitudeOverTime;
			SynthesizedCurve.CurveShared = nullptr;
			SynthesizedCurve.CurveCustom.Reset();
			
			MagFreqCurve.CurveSource = EImpactSynthCurveSource::Generated;
			MagFreqCurve.DisplayName = TEXT("Magnitude–Frequency Distribution (dB)");
			MagFreqCurve.DisplayType = EImpactSynthCurveDisplayType::ScaleOverFrequency;
			MagFreqCurve.CurveShared = nullptr;
			MagFreqCurve.CurveCustom.Reset();
			
			CurveEditor = MakeShared<FCurveEditor>();
			FCurveEditorInitParams InitParams;
			CurveEditor->InitCurveEditor(InitParams);
			CurveEditor->GridLineLabelFormatXAttribute = LOCTEXT("GridXLabelFormat", "{0}");

			TUniquePtr<ICurveEditorBounds> EditorBounds = MakeUnique<FStaticCurveEditorBounds>();
			EditorBounds->SetInputBounds(0.05, 1.05);
			CurveEditor->SetBounds(MoveTemp(EditorBounds));

			CurvePanel = SNew(SCurveEditorPanel, CurveEditor.ToSharedRef());
			
			// Support undo/redo
			InParentObject->SetFlags(RF_Transactional);
			GEditor->RegisterForUndo(this);

			FDetailsViewArgs Args;
			Args.bHideSelectionTip = true;
			Args.NotifyHook = this;

			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertiesView = PropertyModule.CreateDetailView(Args);
			PropertiesView->SetObject(InParentObject);

			TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("ResidualDataEditor_Layoutv1")
			->AddArea
			(
				FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.9f)
					->SetOrientation(Orient_Horizontal)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.225f)
						->AddTab(PropertiesTabId, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewSplitter()
						->SetSizeCoefficient(0.775f)
						->SetOrientation(Orient_Vertical)
						->Split
						(
							FTabManager::NewStack()
							->SetHideTabWell(true)
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

			AddToolbarExtender(CurvePanel->GetToolbarExtender());

			if (CurveEditor.IsValid())
			{
				RegenerateMenusAndToolbars();
			}

			if(UResidualData* ResidualData = Cast<UResidualData>(InParentObject))
			{
				ResidualData->OnResidualDataSynthesized.BindRaw(this, &FResidualDataEditorToolkit::OnResidualDataSynthesized);
				ResidualData->OnStopPlayingResidualData.BindRaw(this, &FResidualDataEditorToolkit::OnStopPlayingResidualData);
				ResidualData->OnResidualDataError.BindRaw(this, &FResidualDataEditorToolkit::OnResidualDataError);
			}
		}

		void FResidualDataEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
		{
			WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_ResidualDataEditor", "Residual Data Editor"));
			FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
			
			InTabManager->RegisterTabSpawner(PropertiesTabId, FOnSpawnTab::CreateSP(this, &FResidualDataEditorToolkit::SpawnTab_Properties))
						.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
						.SetGroup(WorkspaceMenuCategory.ToSharedRef())
						.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ResidualDataEditor.Tabs.Details"));

			FSlateIcon CurveIcon(FAppStyle::GetAppStyleSetName(), "ResidualDataEditor.Tabs.Properties");
			InTabManager->RegisterTabSpawner(CurveTabId, FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs& Args)
			{
				return SpawnTab_OutputCurve(Args);
			}))
			.SetDisplayName(LOCTEXT("ModificationCurvesTab", "Modification Curves"))
			.SetGroup(WorkspaceMenuCategory.ToSharedRef())
			.SetIcon(CurveIcon);
		}

		void FResidualDataEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
		{
			FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
			InTabManager->UnregisterTabSpawner(CurveTabId);
			InTabManager->UnregisterTabSpawner(PropertiesTabId);
		}

		void FResidualDataEditorToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent,
			FProperty* PropertyThatChanged)
		{
			if(PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
				return;

			if(IsCurvePropertyChanged(PropertyThatChanged))
				RefreshCurves();
		}

		bool FResidualDataEditorToolkit::IsCurvePropertyChanged(FProperty* PropertyThatChanged)
		{
			if(PropertyThatChanged)
			{
				const FName PropertyName = PropertyThatChanged->GetFName();
				if (PropertyThatChanged->GetOwnerStruct() == FImpactSynthCurve::StaticStruct()
					|| PropertyName == GET_MEMBER_NAME_CHECKED(UResidualData, AmplitudeOverFreqCurve)
					|| PropertyName == GET_MEMBER_NAME_CHECKED(UResidualData, AmplitudeOverTimeCurve)
					)
					return true;
			}
			return false;
		}

		void FResidualDataEditorToolkit::RefreshCurves()
		{
			check(CurveEditor.IsValid());
			UResidualData* ResidualData = Cast<UResidualData>(GetEditingObject());
			if(ResidualData == nullptr)
				return;
			
			for (const FCurveData& CurveDataEntry : CurveData)
				CurveEditor->UnpinCurve(CurveDataEntry.ModelID);

			// Just reset for safety instead of handling it like in WaveTableEditor
			ResetCurves();

			SetImpactSynthCurve(0, SynthesizedCurve);
			SetImpactSynthCurve(1, ResidualData->AmplitudeOverTimeCurve);
			SetImpactSynthCurve(2, MagFreqCurve);
			SetImpactSynthCurve(3, ResidualData->AmplitudeOverFreqCurve);

			// Collect and remove stale curves from editor
			TArray<FCurveModelID> ToRemove;
			TSet<FCurveModelID> ActiveModelIDs;
			for (const FCurveData& CurveDataEntry : CurveData)
				ActiveModelIDs.Add(CurveDataEntry.ModelID);

			// Remove all dead curves
			const TMap<FCurveModelID, TUniquePtr<FCurveModel>>& Curves = CurveEditor->GetCurves();
			for (const TPair<FCurveModelID, TUniquePtr<FCurveModel>>& Pair : Curves)
			{
				if (!ActiveModelIDs.Contains(Pair.Key))
					ToRemove.Add(Pair.Key);
			}
			for (FCurveModelID ModelID : ToRemove)
				CurveEditor->RemoveCurve(ModelID);
			
			// Re-pin all curves
			for (const FCurveData& CurveDataEntry : CurveData)
				CurveEditor->PinCurve(CurveDataEntry.ModelID);
		}

		void FResidualDataEditorToolkit::SetImpactSynthCurve(const int32 Index, FImpactSynthCurve& ImpactSynthCurve)
		{
			if (!CurveData.IsValidIndex(Index))
				return;;
			
			switch (ImpactSynthCurve.CurveSource)
			{
			case EImpactSynthCurveSource::Shared:
				{
					if (UCurveFloat* SharedCurve = ImpactSynthCurve.CurveShared)
					{
						ClearExpressionCurve(Index);
						SetCurve(Index, SharedCurve->FloatCurve, EImpactSynthCurveSource::Shared);
					}
				}
				break;
				
			case EImpactSynthCurveSource::Custom:
				{
					TrimKeys(ImpactSynthCurve);
					FRichCurve& CustomCurve = ImpactSynthCurve.CurveCustom;
					ClearExpressionCurve(Index);
					SetCurve(Index, CustomCurve, EImpactSynthCurveSource::Custom);
				}
				break;
				
			case EImpactSynthCurveSource::Generated:
				{
					FRichCurve& CustomCurve = ImpactSynthCurve.CurveCustom;
					ClearExpressionCurve(Index);
					SetCurve(Index, CustomCurve, EImpactSynthCurveSource::Generated);
				}
				break;
			default:
				break;
			}
		}
		
		void FResidualDataEditorToolkit::PostUndo(bool bSuccess)
		{
			if (bSuccess)
			{
				RefreshCurves();
			}
		}

		void FResidualDataEditorToolkit::PostRedo(bool bSuccess)
		{
			if (bSuccess)
			{
				RefreshCurves();
			}
		}

		void FResidualDataEditorToolkit::SetCurve(int32 InIndex, FRichCurve& InRichCurve, EImpactSynthCurveSource InSource)
		{
			check(CurveEditor.IsValid());

			if (!ensure(CurveData.IsValidIndex(InIndex)))
				return;
			
			FImpactSynthCurve* ImpactSynthCurve = GetImpactSynthCurve(InIndex);
			if (!ensure(ImpactSynthCurve))
				return;
			
			FCurveData& CurveDataEntry = CurveData[InIndex];
			
			const bool bRequiresNewCurve = RequiresNewCurve(InIndex, InRichCurve);
			if (bRequiresNewCurve)
			{
				TUniquePtr<FImpactSynthCurveModel> NewCurve = ConstructCurveModel(InRichCurve, GetEditingObject(), ImpactSynthCurve);
				NewCurve->Refresh(*ImpactSynthCurve, InIndex);
				CurveDataEntry.ModelID = CurveEditor->AddCurve(MoveTemp(NewCurve));
				CurveEditor->PinCurve(CurveDataEntry.ModelID);
			}
			else
			{
				const TUniquePtr<FCurveModel>& CurveModel = CurveEditor->GetCurves().FindChecked(CurveDataEntry.ModelID);
				check(CurveModel.Get());
				static_cast<FImpactSynthCurveModel*>(CurveModel.Get())->Refresh(*ImpactSynthCurve, InIndex);
			}
		}

		FImpactSynthCurve* FResidualDataEditorToolkit::GetImpactSynthCurve(int32 InIndex)
		{
			if (UResidualData* Bank = Cast<UResidualData>(GetEditingObject()))
			{
				switch (InIndex)
				{
				case 0:
					return &SynthesizedCurve;
				case 1:
					return &Bank->AmplitudeOverTimeCurve;
				case 2:
					return &MagFreqCurve;
				case 3:
					return &Bank->AmplitudeOverFreqCurve;
				default:
					return &SynthesizedCurve;
				}
			}

			return nullptr;
		}

		TUniquePtr<FImpactSynthCurveModel> FResidualDataEditorToolkit::ConstructCurveModel(FRichCurve& InRichCurve, UObject* InParentObject, const FImpactSynthCurve* InSynthCurve)
		{
			return MakeUnique<FImpactSynthCurveModel>(InRichCurve, GetEditingObject(), InSynthCurve->CurveSource, InSynthCurve->DisplayType);
		}

		bool FResidualDataEditorToolkit::SynthesizePreviewedSound(UResidualData* ResidualData, Audio::FAlignedFloatBuffer& SynthesizedData)
		{
			SynthesizedData.Empty();
			
			if(ResidualData == nullptr || ResidualData->ResidualObj == nullptr)
				return false;
			
			if(FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
			{
				const int32 NumChannel = FMath::Min(1, AudioDevice->GetMaxChannels());
				if(NumChannel < 1)
					return false;
				
				const int32 HopSize = ResidualData->NumFFT / 2;
				
				const float PlaySampleRate = AudioDevice->GetSampleRate();
				const auto ResidualDataAssetProxy =  ResidualData->CreateNewResidualProxyData().ToSharedRef();
				FResidualSynth ResidualSynth = FResidualSynth(PlaySampleRate, HopSize, NumChannel, ResidualDataAssetProxy,
													ResidualDataAssetProxy->GetPreviewPlaySpeed(), ResidualData->PreviewVolumeScale,
													GetPitchScaleClamped(ResidualDataAssetProxy->GetPreviewPitchShift()), ResidualDataAssetProxy->GetPreviewSeed(),
													ResidualDataAssetProxy->GetPreviewStartTime(), ResidualDataAssetProxy->GetPreviewDuration());
				
				FResidualSynth::SynthesizeFull(ResidualSynth, SynthesizedData);
				return true;
			}
			
			return false;
		}

		bool FResidualDataEditorToolkit::PlaySynthesizedSound(TArrayView<const float> InData)
		{
			if(!GEditor || !GEditor->CanPlayEditorSound() || InData.Num() == 0)
				return false;
			
			if(FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
			{
				const int32 NumChannel = FMath::Min(1, AudioDevice->GetMaxChannels());
				if(NumChannel < 1)
					return false;
				
				USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>();
				SoundWave->SetSampleRate(AudioDevice->GetSampleRate());
				SoundWave->NumChannels = NumChannel;
				SoundWave->SoundGroup = ESoundGroup::SOUNDGROUP_Effects;
				SoundWave->bLooping = false;
				SoundWave->bCanProcessAsync = false;
				SoundWave->bProcedural = true;
				SoundWave->Duration = InData.Num() / AudioDevice->GetSampleRate();
				
				const EAudioMixerStreamDataFormat::Type Format = SoundWave->GetGeneratedPCMDataFormat();
				if(Format == EAudioMixerStreamDataFormat::Int16)
				{
					TArray<int16_t> OutData;
					OutData.Empty(InData.Num());
					for(int i = 0; i < InData.Num(); i++)
						OutData.Emplace(static_cast<int16_t>(InData[i] * INT16_MAX));

					SoundWave->QueueAudio(reinterpret_cast<uint8*>(OutData.GetData()), OutData.Num() * sizeof(int16_t));
				}
				else
				{
					UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Unsuporrted preview playback format!"));
				}
				GEditor->PlayPreviewSound(SoundWave);
				return true;
			}
			return false;
		}

		void FResidualDataEditorToolkit::GenerateDefaultCurve(FCurveData& OutCurveData, const int32 Index, FImpactSynthCurve& ImpactSynthCurve, const EImpactSynthCurveSource Source)
		{
			TSharedPtr<FRichCurve>& Curve = OutCurveData.ExpressionCurve;
			if (!Curve.IsValid())
				Curve = MakeShared<FRichCurve>();			

			FRichCurve& CurveRef = *Curve.Get();
			TArray<FRichCurveKey> NewKeys;
			NewKeys.Add({ 0.0f, 1.f });
			NewKeys.Add({ 1.0f, 1.f });
			CurveRef.SetKeys(NewKeys);
			SetCurve(Index, *Curve.Get(), EImpactSynthCurveSource::None);
		}

		void FResidualDataEditorToolkit::InitCurves()
		{
			for (int32 i = 0; i < GetNumCurve(); ++i)
			{
				const FImpactSynthCurve* ImpactSynthCurve = GetImpactSynthCurve(i);
				if (!ensure(ImpactSynthCurve))
					continue;

				switch (ImpactSynthCurve->CurveSource)
				{
				case EImpactSynthCurveSource::Shared:
					{
						if (const UCurveFloat* SharedCurve = ImpactSynthCurve->CurveShared)
						{
							if (RequiresNewCurve(i, SharedCurve->FloatCurve))
							{
								ResetCurves();
								return;
							}
						}
						else if (RequiresNewCurve(i, *CurveData[i].ExpressionCurve.Get()))
						{
							ResetCurves();
							return;
						}
					}
					break;

				case EImpactSynthCurveSource::Custom:
					{
						if (RequiresNewCurve(i, ImpactSynthCurve->CurveCustom))
						{
							ResetCurves();
							return;
						}
					}
					break;
					
				default:
					ResetCurves();
					return;
				}
			}
		}

		void FResidualDataEditorToolkit::ResetCurves()
		{
			check(CurveEditor.IsValid());

			CurveEditor->RemoveAllCurves();
			CurveData.Reset();
			CurveData.AddDefaulted(GetNumCurve());
		}

		int32 FResidualDataEditorToolkit::GetNumCurve() const
		{
			return UResidualData::MaxNumCurve + 2; //1 for SynthesizedCurve and 1 for MagFreqCurve
		}

		TSharedRef<SDockTab> FResidualDataEditorToolkit::SpawnTab_OutputCurve(const FSpawnTabArgs& Args)
		{
			FAlignedFloatBuffer TempData;
			SynthesizeDataAndCurves(TempData);
			RefreshCurves();
			CurveEditor->ZoomToFitAll();
			
			TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
				.Label(FText::Format(LOCTEXT("ResidualDataCurveEditorTitle", "Modification Curve: {0}"), FText::FromString(GetEditingObject()->GetName())))
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

		TSharedRef<SDockTab> FResidualDataEditorToolkit::SpawnTab_Properties(const FSpawnTabArgs& Args)
		{
			check(Args.GetTabId() == PropertiesTabId);

			return SNew(SDockTab)
				.Label(LOCTEXT("ResidualDataDetailsTitle", "Details"))
				[
					PropertiesView.ToSharedRef()
				];
		}

		void FResidualDataEditorToolkit::ClearExpressionCurve(int32 InIndex)
		{
			if (CurveData.IsValidIndex(InIndex))
			{
				CurveData[InIndex].ExpressionCurve.Reset();
			}
		}

		void FResidualDataEditorToolkit::TrimKeys(FImpactSynthCurve& ImpactSynthCurve)
		{
			double MinTime;
			double MaxTime;
			if(FImpactSynthCurve::TryGetTimeRangeX(ImpactSynthCurve.DisplayType, MinTime, MaxTime))
			{
				while (ImpactSynthCurve.CurveCustom.GetNumKeys() > 0 && ImpactSynthCurve.CurveCustom.GetFirstKey().Time < MinTime)
				{
					ImpactSynthCurve.CurveCustom.DeleteKey(ImpactSynthCurve.CurveCustom.GetFirstKeyHandle());
				}

				while (ImpactSynthCurve.CurveCustom.GetNumKeys() > 0 && ImpactSynthCurve.CurveCustom.GetLastKey().Time > MaxTime)
				{
					ImpactSynthCurve.CurveCustom.DeleteKey(ImpactSynthCurve.CurveCustom.GetLastKeyHandle());
				}
			}
		}

		bool FResidualDataEditorToolkit::RequiresNewCurve(int32 InIndex, const FRichCurve& InRichCurve) const
		{
			const FCurveModelID CurveModelID = CurveData[InIndex].ModelID;
			const TUniquePtr<FCurveModel>* CurveModel = CurveEditor->GetCurves().Find(CurveModelID);
			if (!CurveModel || !CurveModel->IsValid())
			{
				return true;
			}

			FImpactSynthCurveModel* PatchCurveModel = static_cast<FImpactSynthCurveModel*>(CurveModel->Get());
			check(PatchCurveModel);
			if (&PatchCurveModel->GetRichCurve() != &InRichCurve)
			{
				return true;
			}

			return false;
		}

		void FResidualDataEditorToolkit::OnResidualDataSynthesized() 
		{
			if(!GEditor || !GEditor->CanPlayEditorSound())
				return;

			Audio::FAlignedFloatBuffer SynthesizedData;
			SynthesizeDataAndCurves(SynthesizedData);			
			PlaySynthesizedSound(SynthesizedData);
		}

		void FResidualDataEditorToolkit::SynthesizeDataAndCurves(Audio::FAlignedFloatBuffer& SynthesizedData)
		{
			SynthesizedData.Empty();
			SynthesizedCurve.CurveCustom.Reset();
			MagFreqCurve.CurveCustom.Reset();
			
			UResidualData* ResidualData = Cast<UResidualData>(GetEditingObject());
			if(ResidualData == nullptr || ResidualData->ResidualObj == nullptr)
			{
				return;
			}
			
			if(FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
			{
				const int32 NumChannel = FMath::Min(1, AudioDevice->GetMaxChannels());
				if(NumChannel < 1)
					return;
				
				const int32 HopSize = ResidualData->NumFFT / 2;
				
				const float PlaySampleRate = AudioDevice->GetSampleRate();
				const auto ResidualDataAssetProxy =  ResidualData->CreateNewResidualProxyData().ToSharedRef();
				const float StartTime = ResidualDataAssetProxy->GetPreviewStartTime();
				FResidualSynth ResidualSynth = FResidualSynth(PlaySampleRate, HopSize, NumChannel, ResidualDataAssetProxy,
															  ResidualDataAssetProxy->GetPreviewPlaySpeed(),
															  ResidualData->PreviewVolumeScale,
															  GetPitchScaleClamped(ResidualDataAssetProxy->GetPreviewPitchShift()),
															  ResidualDataAssetProxy->GetPreviewSeed(),
															  StartTime,
															  ResidualDataAssetProxy->GetPreviewDuration(),
															  false);
				
				int32 NumOutSamples =  FMath::CeilToInt32(ResidualSynth.GetMaxDuration() * PlaySampleRate);
				NumOutSamples = FMath::Max(HopSize, NumOutSamples);
				
				SynthesizedData.SetNumUninitialized(NumOutSamples);
				FMemory::Memzero(SynthesizedData.GetData(), NumOutSamples * sizeof(float));

				FAlignedFloatBuffer MagFreq;
				MagFreq.SetNumUninitialized(ResidualSynth.GetCurrentMagFreq().Num());
				FMemory::Memzero(MagFreq.GetData(), MagFreq.Num() * sizeof(float));
				
				FMultichannelBufferView OutAudioView;
				OutAudioView.Emplace(SynthesizedData);
				int32 CurrentFrame = 0;
				bool bIsFinished = false;
				int32 EndIdx = 0;
				const int32 HalfHop = HopSize /2;
				float CurrentTime = StartTime;
				while (!bIsFinished && EndIdx <= NumOutSamples)
				{
					FMultichannelBufferView BufferView = SliceMultichannelBufferView(OutAudioView, CurrentFrame * HopSize, HopSize);
					bIsFinished = ResidualSynth.Synthesize(BufferView, false, true);
					
					float Magnitude = Audio::ArrayGetMagnitude(BufferView[0].Slice(0, HalfHop));
					SynthesizedCurve.CurveCustom.AddKey(CurrentTime, Magnitude, false);
					const float HalfDeltaTime = (ResidualSynth.GetCurrentTIme() - CurrentTime) / 2.0f;
					CurrentTime += HalfDeltaTime;
					
					Magnitude = Audio::ArrayGetMagnitude(BufferView[0].Slice(HalfHop, HalfHop));
					SynthesizedCurve.CurveCustom.AddKey(CurrentTime, Magnitude, false);
					CurrentTime += HalfDeltaTime;
					
					CurrentFrame++;
					EndIdx = CurrentFrame * HopSize + HopSize;
					
					//This will not take into account the first frame as ResidualSynth.Synthesize will run 2 times at the start
					Audio::ArrayAddInPlace(ResidualSynth.GetCurrentMagFreq(), MagFreq);
				}

				AddMagFreqCurveKeys(ResidualSynth, MagFreq);
			}
		}

		void FResidualDataEditorToolkit::AddMagFreqCurveKeys(const LBSImpactSFXSynth::FResidualSynth& ResidualSynth, TArrayView<const float> MagFreq)
		{
			const float FreqResolution = ResidualSynth.GetFreqResolution();
			float FinalFreqMag = 0;
			TArray<float> MagFreqDb;
			TArray<float> FreqBins;
			float MaxDb = -1e6f;
			MagFreqDb.Empty(MagFreq.Num());
			FreqBins.Empty(MagFreq.Num());
			for(int i = 0; i < MagFreq.Num(); i++)
			{
				const float FreqBin = UResidualData::Freq2Erb(i * FreqResolution) / FImpactSynthCurve::ErbMax;
				if(FreqBin >= 1.0f)
					FinalFreqMag += MagFreq[i];
				else
				{
					FreqBins.Emplace(FreqBin);
					const float Value = 20 * FMath::LogX(10.0f, MagFreq[i]);
					MagFreqDb.Emplace(Value);
					if(Value > MaxDb)
						MaxDb = Value;
				}
			}

			FinalFreqMag = 20 * FMath::LogX(10.0f, FinalFreqMag);
			FreqBins.Emplace(1.0f);
			MagFreqDb.Emplace(FinalFreqMag);
			if(FinalFreqMag > MaxDb)
				MaxDb = FinalFreqMag;

			float PreValue = 0;
			for(int i = 0; i < MagFreqDb.Num(); i++)
			{
				const float Value = FMath::Max(-100.0f, MagFreqDb[i] - MaxDb);
				if(i == 0 || Value != PreValue)
				{
					const FKeyHandle Handle = MagFreqCurve.CurveCustom.AddKey(FreqBins[i], Value, false);
					MagFreqCurve.CurveCustom.GetKey(Handle).InterpMode = ERichCurveInterpMode::RCIM_Constant;
					PreValue = Value;
				}
			}
		}

		void FResidualDataEditorToolkit::OnStopPlayingResidualData()
		{
			if(GEditor)
				GEditor->ResetPreviewAudioComponent();
		}

		void FResidualDataEditorToolkit::OnResidualDataError(const FString& InTitle, const FString& InMessage)
		{
			FText Title = FText::FromString(InTitle);
			FText Message = FText::FromString(InMessage);
			UEditorDialogLibrary::ShowMessage(Title, Message, EAppMsgType::Ok);
		}
	}
}

#undef LOCTEXT_NAMESPACE