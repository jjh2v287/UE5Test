// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "MultiImpactSynth.h"

#include "ImpactModalObj.h"
#include "ModalSynth.h"
#include "ResidualSynth.h"
#include "DSP/FloatArrayMath.h"
#include "ImpactSFXSynth/Public/Utils.h"

namespace LBSImpactSFXSynth
{
	FMultiImpactSynth::FMultiImpactSynth(FMultiImpactDataAssetProxyRef InDataAssetProxyRef, const int32 InMaxNumImpacts,
										 const int32 InSamplingRate, const int32 InNumFramesPerBlock, const int32 InNumChannels,
										 const bool InIsStopSpawningOnMax, const int32 InSeed, EMultiImpactVariationSpawnType InSpawnType,
										 const int32 Slack)
		: MultiImpactProxy(InDataAssetProxyRef), MaxNumImpacts(InMaxNumImpacts), SamplingRate(InSamplingRate)
		, NumFramesPerBlock(InNumFramesPerBlock), NumChannels(InNumChannels), bIsStopSpawningOnMax(InIsStopSpawningOnMax)
		, VariationSpawnType(InSpawnType), GlobalDecayScale(1.f), GlobalResidualSpeedScale(1.f), GlobalModalPitchShift(0.f), GlobalResidualPitchShift(0.f)
	{
		const FImpactModalObjAssetProxyPtr& ImpactModalProxy = MultiImpactProxy->GetModalProxy();
		bHasModalSynth = ImpactModalProxy.IsValid();
		if(bHasModalSynth)
			ModalSynths.Empty(Slack);
		NumUsedModals = MultiImpactProxy->GetNumUsedModals();
		
		const FResidualDataAssetProxyPtr& ResidualDataProxy = MultiImpactProxy->GetResidualProxy();
		bHasResidualSynth = ResidualDataProxy.IsValid();
		if(bHasResidualSynth)
			ResidualSynths.Empty(Slack);

		Seed = InSeed > -1 ? InSeed : FMath::Rand(); 
		RandomStream = FRandomStream(Seed);		
		
		Restart();
	}

	void FMultiImpactSynth::Restart()
	{
		NextFrameTime = 0.f;
		AccumNoSpawnTime = 0.f;
		
		NumActiveModalSynths = 0;
		NumActiveResidualSynths = 0;
		
		CurrentSpawnChance = 1.f;
		LastVariationIndex = -1;
		bIsRevert = false;
	}

	bool FMultiImpactSynth::Synthesize(FMultichannelBufferView& OutAudioView, const FMultiImpactSpawnParams& SpawnParams,
									  const float InGlobalDecayScale, const float InGlobalResidualSpeedScale,
									  const float InGlobalModalPitchShift, const float InGlobalResidualPitchShift,
									  const bool bClamp)
	{
		check(NumChannels == OutAudioView.Num());
		const int32 NumFramesToGenerate = OutAudioView[0].Num();
		checkf(NumFramesToGenerate <= NumFramesPerBlock, TEXT("FMultiImpactSynth::Synthesize: NumFramesPerBlock is smaller than the requested number of frames."));

		GlobalDecayScale = InGlobalDecayScale;
		GlobalResidualSpeedScale = InGlobalResidualSpeedScale;
		GlobalModalPitchShift = InGlobalModalPitchShift;
		GlobalResidualPitchShift = InGlobalResidualPitchShift;
		
		const bool bIsStopSpawningNewImpact = SpawningNewImpacts(NumFramesToGenerate, SpawnParams);
		
		// For modal synth, only synth one channel since all channels have the same data
		FMultichannelBufferView FirstChannelBuffer;
		FirstChannelBuffer.Emplace(OutAudioView[0]);
		NumActiveModalSynths = 0;
		for(int i = 0; i < ModalSynths.Num(); i++)
		{
			if(!ModalSynths[i]->IsFinished())
			{
				const bool bIsFinished = ModalSynths[i]->Synthesize(FirstChannelBuffer, MultiImpactProxy->GetModalProxy(), true, false);
				NumActiveModalSynths += !bIsFinished;
			}
		}
		
		//Propagate data from first channel to others
		if(bHasModalSynth)
		{
			const float* CurrentData = FirstChannelBuffer[0].GetData();
			for(int i = 1; i < NumChannels; i++)
			{
				FMemory::Memcpy(OutAudioView[i].GetData(), CurrentData, NumFramesToGenerate * sizeof(float));
			}
		}

		NumActiveResidualSynths = 0;
		for(int i = 0; i < ResidualSynths.Num(); i++)
		{
			if(!ResidualSynths[i]->IsFinished())
			{
				const bool bIsFinished = ResidualSynths[i]->Synthesize(OutAudioView, true, false);
				NumActiveResidualSynths += !bIsFinished;
			}
		}

		if(bClamp)
		{
			for(int32 Channel = 0; Channel < NumChannels; Channel++)
				Audio::ArrayClampInPlace(OutAudioView[Channel], -1.f, 1.f);			
		}
		
		const bool bIsStopSpawningImpact = SpawnParams.ImpactSpawnDuration >= 0 ? (NextFrameTime >= SpawnParams.ImpactSpawnDuration) : bIsStopSpawningNewImpact;
		if(NumActiveModalSynths == 0 && NumActiveResidualSynths == 0 && bIsStopSpawningImpact)
			return true;		

		return false;
	}

	bool FMultiImpactSynth::SpawningNewImpacts(const int32 NumFramesToGenerate, const FMultiImpactSpawnParams& SpawnParams)
	{
		const float DeltaTime = NumFramesToGenerate / SamplingRate;
		const float CurrentTime = NextFrameTime;
		NextFrameTime += DeltaTime;
		
		if(!bHasModalSynth && !bHasResidualSynth)
			return true;
		
		if(SpawnParams.ImpactSpawnDuration >= 0.f && CurrentTime > SpawnParams.ImpactSpawnDuration)
			return true;
			
		const int32 NumActiveImpacts = FMath::Max(NumActiveModalSynths, NumActiveResidualSynths);
		if(NumActiveImpacts >= MaxNumImpacts && bIsStopSpawningOnMax)
			return true;
			
		CurrentSpawnChance = SpawnParams.ImpactSpawnChance * FMath::Exp(-SpawnParams.ImpactChanceDecayRate * CurrentTime);
		if(CurrentSpawnChance < 1e-3)
			return true;

		if(CurrentTime > 0.f && (SpawnParams.ImpactSpawnRate == 0.f || SpawnParams.ImpactSpawnCount < 1))
			return true;

		const float DecayFactor = FMath::Exp (-SpawnParams.ImpactStrengthDecayRate * CurrentTime);
		const float CurrentMaxImpactStrength = (SpawnParams.ImpactStrengthMin + SpawnParams.ImpactStrengthRange) * DecayFactor;
		if(CurrentMaxImpactStrength < 1e-3)
			return true;
		
		const TArrayView<const FImpactSpawnInfo> SpawnInfos = MultiImpactProxy->GetSpawnInfos();
		const int32 NumInfos = SpawnInfos.Num();
		VariationIndexes.Empty(NumInfos);
		for(int i = 0; i < NumInfos; i++)
		{
			if(SpawnInfos[i].CanSpawn(CurrentTime))
				VariationIndexes.Emplace(i);
		}
		if(VariationIndexes.Num() == 0)
			return true;
		
		int32 NumImpactToSpawn;
		if(CurrentTime == 0.f)
		{
			NumImpactToSpawn = SpawnParams.ImpactSpawnBurst;
		}
		else
		{
			AccumNoSpawnTime += DeltaTime;
			const float SpawnRateAccum = AccumNoSpawnTime * SpawnParams.ImpactSpawnRate;
			if(SpawnRateAccum < 1.f)
				return false;
			NumImpactToSpawn = FMath::RoundToInt32(SpawnRateAccum * SpawnParams.ImpactSpawnCount);
		}

		AccumNoSpawnTime = 0.f;
		const int32 NumRemainImpactCanSpawn = MaxNumImpacts - NumActiveImpacts;
		if(NumRemainImpactCanSpawn < NumImpactToSpawn && !bIsStopSpawningOnMax)
			StopWeakestSynths(NumImpactToSpawn);
		
		const FImpactModalObjAssetProxyPtr& ModalParamsPtr = MultiImpactProxy->GetModalProxy();
		const FResidualDataAssetProxyPtr& ResidualDataProxy = MultiImpactProxy->GetResidualProxy();
			
		const float CurrentMinImpactStrength = SpawnParams.ImpactStrengthMin * DecayFactor;
		const float CurrentImpactStrengthRange = SpawnParams.ImpactStrengthRange * DecayFactor;
		const bool bIsRandomlyGetModal = MultiImpactProxy->IsRandomlyGetModals();
		int32 NumNewSpawn = 0;
		for(int i = 0; i < NumImpactToSpawn && NumNewSpawn < NumRemainImpactCanSpawn; i++)
		{
			if(!FMath::IsNearlyEqual(CurrentSpawnChance, 1.0f) && (RandomStream.FRand() > CurrentSpawnChance))
				continue;

			const FImpactSpawnInfo* SpawnInfo = GeVariationSpawnInfo(SpawnInfos);

			//Make sure always have one impact at time 0 
			const float DelayStartTime = (NumNewSpawn > 0 || CurrentTime > 0.f) ? GetRandRange(RandomStream, 0.f, SpawnParams.ImpactSpawnDelay) : 0.f;

			NumNewSpawn++;
			
			float ImpactStrength = GetRandRange(RandomStream, CurrentMinImpactStrength, CurrentImpactStrengthRange);
			ImpactStrength = GetImpactStrengthScaleClamped(ImpactStrength);
			if(ImpactStrength < 1e-5f)
				continue;
			
			if(bHasModalSynth && SpawnInfo->bUseModalSynth)
			{
				const float AmpScale = SpawnInfo->GetModalAmplitudeScaleRand(RandomStream);
				const float DecayScale = SpawnInfo->GetModalDecayScaleRand(RandomStream, GlobalDecayScale);
				const float PitchScale = SpawnInfo->GetModalPitchScaleRand(RandomStream, GlobalModalPitchShift);
				
				bool bIsNoReuse = true;
				for(int m = 0; m < ModalSynths.Num(); m++)
				{
					if(!ModalSynths[m]->IsRunning())
					{
						ModalSynths[m]->ResetAllStates(ModalParamsPtr,
															SpawnInfo->ModalStartTime, SpawnInfo->ModalDuration,
															AmpScale, DecayScale, PitchScale,
															ImpactStrength, SpawnInfo->DampingRatio, bIsRandomlyGetModal, DelayStartTime);
						bIsNoReuse = false;
						break;
					}
				}
					
				if(bIsNoReuse)
				{
					ModalSynths.Emplace(MakeShared<FModalSynth>(SamplingRate,
																ModalParamsPtr, SpawnInfo->ModalStartTime, SpawnInfo->ModalDuration,
																NumUsedModals,AmpScale,
																DecayScale, PitchScale, ImpactStrength,
																SpawnInfo->DampingRatio, DelayStartTime, bIsRandomlyGetModal));
				}
			}
				
			if(bHasResidualSynth && SpawnInfo->bUseResidualSynth)
			{
				const float AmpScale = SpawnInfo->GetResidualAmplitudeScaleRand(RandomStream);
				const float PlaySpeed = SpawnInfo->GetResidualPlaySpeedScaleRand(RandomStream, GlobalResidualSpeedScale);
				const float PitchScale = SpawnInfo->GetResidualPitchScaleRand(RandomStream, GlobalResidualPitchShift);

				bool bIsNoReuse = true;
				for(int r = 0; r < ResidualSynths.Num(); r++)
				{
					if(!ResidualSynths[r]->IsRunning())
					{
						ResidualSynths[r]->ChangeScalingParams(SpawnInfo->ResidualStartTime, SpawnInfo->ResidualDuration,
																PlaySpeed, AmpScale, PitchScale, ImpactStrength);
						ResidualSynths[r]->Restart(DelayStartTime);
						bIsNoReuse = false;
						break;
					}
				}

				if(bIsNoReuse)
				{
					ResidualSynths.Emplace(MakeShared<FResidualSynth>(SamplingRate, NumFramesPerBlock, NumChannels,
																	   ResidualDataProxy.ToSharedRef(), PlaySpeed, AmpScale, PitchScale,
																	   -1, SpawnInfo->ResidualStartTime, SpawnInfo->ResidualDuration, false,
																	   ImpactStrength, false, DelayStartTime));
				}
			}
				
		}

		return false;
	}

	const FImpactSpawnInfo* FMultiImpactSynth::GeVariationSpawnInfo(const TArrayView<const FImpactSpawnInfo> SpawnInfos)
	{
		switch(VariationSpawnType)
		{
			case EMultiImpactVariationSpawnType::Increment:
			{
				LastVariationIndex++;
				if(LastVariationIndex < 0 || LastVariationIndex >= VariationIndexes.Num())
					LastVariationIndex = 0;
			}
			break;

			case EMultiImpactVariationSpawnType::Decrement:
			{
				LastVariationIndex--;
            	if(LastVariationIndex < 0 || LastVariationIndex >= VariationIndexes.Num())
            		LastVariationIndex = VariationIndexes.Num() - 1;
			}
			break;

			case EMultiImpactVariationSpawnType::PingPong:
			{
				if(bIsRevert)
				{
					if(LastVariationIndex < 1)
					{
						LastVariationIndex = 1;
						bIsRevert = false;
					}
					else
						LastVariationIndex--;
				}
				else
				{
					const int32 NearEndIdx = VariationIndexes.Num() - 2;
					if(LastVariationIndex > NearEndIdx)
					{
						LastVariationIndex = NearEndIdx;
						bIsRevert = true;
					}
					else
						LastVariationIndex++;
				}
					
				LastVariationIndex = FMath::Clamp(LastVariationIndex, 0, VariationIndexes.Num() - 1); 
			}
			break;
			
			case EMultiImpactVariationSpawnType::Random:
			default:
				LastVariationIndex = RandomStream.RandRange(0, VariationIndexes.Num() - 1);
				break;
		}

		
		return 	&SpawnInfos[VariationIndexes[LastVariationIndex]];
	}
	
	void FMultiImpactSynth::StopWeakestSynths(const int32 NumImpactToSpawn)
	{
		const int32 NumRemainModalSynths = MaxNumImpacts - NumActiveModalSynths;
		const int32 NumModalSynthToStop = NumImpactToSpawn - NumRemainModalSynths;
		TArray<int32> Indexes;
		if(NumModalSynthToStop > 0)
		{
			Indexes.Empty(NumActiveModalSynths);
			for(int i = 0; i < ModalSynths.Num(); i++)
			{
				if(ModalSynths[i]->IsRunning())
					Indexes.Emplace(i);
			}

			Algo::Sort(Indexes, [this](const int32 A, const int32 B)
			{
				return ModalSynths[A]->GetCurrentMaxAmplitude() < ModalSynths[B]->GetCurrentMaxAmplitude();
			});

			const int32 NumStop = FMath::Min(NumModalSynthToStop, Indexes.Num());
			for(int i = 0; i < NumStop; i++)
				ModalSynths[Indexes[i]]->ForceStop();
		}
			
		const int32 NumRemainResidualSynths = MaxNumImpacts - NumActiveResidualSynths;
		const int32 NumResidualSynthToStop = NumImpactToSpawn - NumRemainResidualSynths;
		if(NumRemainResidualSynths < NumImpactToSpawn)
		{
			Indexes.Empty(NumActiveResidualSynths);
			for(int i = 0; i < ResidualSynths.Num(); i++)
			{
				if(ResidualSynths[i]->IsRunning())
					Indexes.Emplace(i);
			}
			
			Algo::Sort(Indexes, [this](const int32 A, const int32 B)
			{
				return ResidualSynths[A]->GetCurrentFrameEnergy() < ResidualSynths[B]->GetCurrentFrameEnergy();
			});

			const int32 NumStop = FMath::Min(NumResidualSynthToStop, Indexes.Num());
			for(int i = 0; i < NumStop; i++)
				ResidualSynths[Indexes[i]]->ForceStop();
		}
	}

	void FMultiImpactSynth::StopAllSynthesizers()
	{
		NumActiveModalSynths = 0;
		for(int i = 0; i < ModalSynths.Num(); i++)
			ModalSynths[i]->ForceStop();

		NumActiveResidualSynths = 0;
		for(int i = 0; i < ResidualSynths.Num(); i++)
			ResidualSynths[i]->ForceStop();
	}

	float FMultiImpactSynth::GetImpactStrengthScaleClamped(const float InValue) const
	{
		return FMath::Clamp(InValue, 0.f, 10.f);
	}
}
