// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ScratchingSynth.h"

#include "CustomStatGroup.h"
#include "ImpactSFXSynthLog.h"
#include "ResidualSynth.h"
#include "ImpactSFXSynth/Public/Utils.h"
#include "DSP/AudioFFT.h"
#include "DSP/FloatArrayMath.h"

DECLARE_CYCLE_STAT(TEXT("ScratchingSynth - Synthesize"), STAT_ScratchingSynth, STATGROUP_ImpactSFXSynth);

namespace LBSImpactSFXSynth
{
	FScratchingSynth::FScratchingSynth(const float InSamplingRate, const int32 InNumFramesPerBlock,
		const int32 InNumOutChannel, const int32 InSeed, const FResidualDataAssetProxyPtr& ResidualDataProxyPtr,
		const float ResidualPlaySpeed, const float ResidualAmplitudeScale, const float ResidualPitchScale,
		const float ResidualStartTime, const float ResidualDuration)
	: SamplingRate(InSamplingRate), NumOutChannel(InNumOutChannel),
	  NumFFT(1024), CurrentSynthBufferIndex(0), CurrentFrame(0), LastImpactSpawnTime(0.f), bIsFinalFrameSynth(false)
	{	
		if(InSeed > -1)
			Seed = InSeed;
		else
			Seed = FMath::Rand();
		ImpactRandomStream = FRandomStream(Seed);
		
		if(ResidualDataProxyPtr.IsValid())
		{
			FResidualDataAssetProxyRef ResidualDataAssetProxyRef = ResidualDataProxyPtr.ToSharedRef();
			if(ResidualDataAssetProxyRef->GetResidualData())
			{
				ResidualSynth = MakeUnique<FResidualSynth>(InSamplingRate, InNumFramesPerBlock, InNumOutChannel, ResidualDataAssetProxyRef,
															ResidualPlaySpeed, ResidualAmplitudeScale, ResidualPitchScale, -1,
															ResidualStartTime, ResidualDuration);
				NumFFT = ResidualDataAssetProxyRef->GetNumFFT();
			}
			
		}
		
		if(!ResidualSynth.IsValid())
			IniFFTBuffers();

		HopSize = NumFFT / 2;
		TimeStep = HopSize / InSamplingRate;

		MagBuffer.SetNumUninitialized(NumFFT / 2 + 1);
	}

	bool FScratchingSynth::Synthesize(const FScratchImpactSpawnParams& SpawnParams, const FImpactModalObjAssetProxyPtr& ExciteModalsParams, const FModalMods& ExciteModalMods,
		const FImpactModalObjAssetProxyPtr& ResonModalsParams, const FModalMods& ResonModalMods, FMultichannelBufferView& OutAudio,
		bool bClampOutput)
	{
		SCOPE_CYCLE_COUNTER(STAT_ScratchingSynth);

		if(bIsFinalFrameSynth)
			return true;
		
		int32 NumRemainOutputFrames = Audio::GetMultichannelBufferNumFrames(OutAudio);
		checkf(NumOutChannel == OutAudio.Num(), TEXT("FScratchingSynth::Synthesize: Expect %d channels but requested %d channels"), NumOutChannel, OutAudio.Num());
		
		if(!ExciteModalsParams.IsValid() && !ResonModalsParams.IsValid() && !ResidualSynth.IsValid())
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FScratchingSynth::Synthesize: no data to synthesize!"));
			return true;
		}

		TArrayView<FAlignedFloatBuffer> SynthesizedDataBuffersView = ResidualSynth.IsValid() ? ResidualSynth->GetSynthesizedDataBuffers()
																	 :  TArrayView<FAlignedFloatBuffer>(SynthesizedDataBuffers);
		TArrayView<FAlignedFloatBuffer> OutRealsView = ResidualSynth.IsValid() ? ResidualSynth->GetOutRealBuffer()
																	 :  TArrayView<FAlignedFloatBuffer>(OutReals);
		int32 CurrentOutIdx = 0;
		const int32 HalfSize = OutRealsView[0].Num() / 2;
		if(CurrentFrame == 0)
		{
			bIsFinalFrameSynth = SynthesizeOneFrame(SpawnParams, ExciteModalsParams, ExciteModalMods, ResonModalsParams, ResonModalMods);
			for(int32 Channel = 0; Channel < NumOutChannel; Channel++)
			{
				const TArrayView<const float> OutFirstHalf = TArrayView<const float>(OutRealsView[Channel]).Slice(0, HalfSize);
				FMemory::Memcpy(SynthesizedDataBuffersView[Channel].GetData(), OutFirstHalf.GetData(), HalfSize * sizeof(float));
			}
			CurrentSynthBufferIndex = 0;	
		}
		
		while (NumRemainOutputFrames > 0)
		{
			checkf(CurrentSynthBufferIndex <= SynthesizedDataBuffersView[0].Num(), TEXT("FScratchingSynth::Synthesize: CurrentSynthBufferIndex is outside of buffer size!"))
			if(CurrentSynthBufferIndex >= SynthesizedDataBuffersView[0].Num())
			{
				if(bIsFinalFrameSynth) //No frame to synth anymore
					break;
				
				for(int32 Channel = 0; Channel < NumOutChannel; Channel++)
				{
					TArrayView<const float> OutLastHalf = TArrayView<const float>(OutRealsView[Channel]).Slice(HalfSize, HalfSize);
					FMemory::Memcpy(SynthesizedDataBuffersView[Channel].GetData(), OutLastHalf.GetData(), HalfSize * sizeof(float));
				}
				
				bIsFinalFrameSynth = SynthesizeOneFrame(SpawnParams, ExciteModalsParams, ExciteModalMods, ResonModalsParams, ResonModalMods);
				for(int32 Channel = 0; Channel < NumOutChannel; Channel++)
				{
					const TArrayView<const float> OutFirstHalf = TArrayView<const float>(OutRealsView[Channel]).Slice(0, HalfSize);
					Audio::ArrayAddInPlace(OutFirstHalf, SynthesizedDataBuffersView[Channel]);
				}
				CurrentSynthBufferIndex = 0;
			}
			
			const int32 NumRemainSynthData = SynthesizedDataBuffersView[0].Num() - CurrentSynthBufferIndex;
			const int32 NumCopyFrame = FMath::Min(NumRemainOutputFrames, NumRemainSynthData);
			
			for(int32 Channel = 0; Channel < NumOutChannel; Channel++)
			{
				auto OutDest = OutAudio[Channel].Slice(CurrentOutIdx, NumCopyFrame);
				auto InSource = TArrayView<const float>(SynthesizedDataBuffersView[Channel]).Slice(CurrentSynthBufferIndex, NumCopyFrame);
				FMemory::Memcpy(OutDest.GetData(), InSource.GetData(), NumCopyFrame * sizeof(float));
			}
			
			CurrentOutIdx += NumCopyFrame;
			NumRemainOutputFrames -= NumCopyFrame;
			CurrentSynthBufferIndex += NumCopyFrame;
		}
		
		if(bClampOutput)
		{
			for(int32 Channel = 0; Channel < NumOutChannel; Channel++)
			{
				Audio::ArrayClampInPlace(OutAudio[Channel], -1.f, 1.f);
			}
		}
		
		return bIsFinalFrameSynth;
	}

	void FScratchingSynth::IniFFTBuffers()
	{
		if(!IsPowerOf2(NumFFT))
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FScratchingSynth::IniFFTBuffers: NumFFT = %d is not a power of 2!"), NumFFT);
			return;
		}

		FFFTSettings FFTSettings;
		FFTSettings.Log2Size = Audio::CeilLog2(NumFFT);
		FFTSettings.bArrays128BitAligned = true;
		FFTSettings.bEnableHardwareAcceleration = true;
		
		checkf(FFFTFactory::AreFFTSettingsSupported(FFTSettings), TEXT("No fft algorithm supports fft settings."));
		FFT = FFFTFactory::NewFFTAlgorithm(FFTSettings);
		if(FFT.IsValid())
		{
			ConjBuffers.Empty(2);
			ConjBuffers.Emplace(FAlignedFloatBuffer());
			ConjBuffers.Emplace(FAlignedFloatBuffer());
			ConjBuffers[0].SetNumUninitialized(FFT->NumOutputFloats() / 2);
			ConjBuffers[1].SetNumUninitialized(FFT->NumOutputFloats() / 2);
			ComplexSpectrum.SetNumUninitialized(FFT->NumOutputFloats());

			OutReals.Empty(NumOutChannel);
			SynthesizedDataBuffers.Empty(NumOutChannel);
			for(int i = 0; i < NumOutChannel; i++)
			{
				OutReals.Emplace(FAlignedFloatBuffer());
				OutReals[i].SetNumUninitialized(FFT->NumInputFloats());

				SynthesizedDataBuffers.Emplace(FAlignedFloatBuffer());
				SynthesizedDataBuffers[i].SetNumUninitialized(FFT->NumInputFloats() / 2);
			}
			
			WindowBuffer.SetNumUninitialized(FFT->NumInputFloats());
			FResidualSynth::GenerateHannWindow(WindowBuffer.GetData(), FFT->NumInputFloats(), true);
		}
	}

	void FScratchingSynth::DoIFFT()
	{
		const int32 NumPoints = ConjBuffers[0].Num();
		for(int i = 0; i < NumPoints; i++)
		{
			const int32 PhaseIndex = FMath::RandHelper(FResidualStats::NumSinPoint);
			ConjBuffers[0][i] = FResidualStats::SinCycle[(PhaseIndex + FResidualStats::CosShift) % FResidualStats::NumSinPoint];
			ConjBuffers[1][i] = FResidualStats::SinCycle[PhaseIndex];
		}
		
		Audio::ArrayMultiplyInPlace(MagBuffer, ConjBuffers[0]);
		Audio::ArrayMultiplyInPlace(MagBuffer, ConjBuffers[1]);
		
		Audio::ArrayInterleave(ConjBuffers, ComplexSpectrum);
		FFT->InverseComplexToReal(ComplexSpectrum.GetData(), OutReals[0].GetData());
		ArrayMultiplyInPlace(WindowBuffer, OutReals[0]);

		if(NumOutChannel > 1)
		{
			TArray<const float*> BufferPtrArray;
			BufferPtrArray.Reset(2);
			BufferPtrArray.Emplace(ConjBuffers[1].GetData());
			BufferPtrArray.Emplace(ConjBuffers[0].GetData());
			Audio::ArrayInterleave(BufferPtrArray.GetData(), ComplexSpectrum.GetData(), ConjBuffers[0].Num(), 2);
			FFT->InverseComplexToReal(ComplexSpectrum.GetData(), OutReals[1].GetData());
			ArrayMultiplyInPlace(WindowBuffer, OutReals[1]);
		}
	}

	bool FScratchingSynth::SynthesizeOneFrame(const FScratchImpactSpawnParams& SpawnParams, const FImpactModalObjAssetProxyPtr& ExciteModalsParams,
	                                          const FModalMods& ExciteModalMods, const FImpactModalObjAssetProxyPtr& ResonModalsParams, const FModalMods& ResonModalMods)
	{
		const bool bCanStillSpawn = SpawnImpactIfNeeded(SpawnParams, ExciteModalsParams, ExciteModalMods, ResonModalsParams, ResonModalMods);
		SynthesizeCurrentImpacts(ExciteModalsParams, ResonModalsParams);
		
		CurrentFrame++;

		if(!bCanStillSpawn && ImpactInfos.Num() <= 0)
			return true;
		else
			return false;
	}

	bool FScratchingSynth::SpawnImpactIfNeeded(const FScratchImpactSpawnParams& SpawnParams, const FImpactModalObjAssetProxyPtr& ExciteModalsParams,
											   const FModalMods& ExciteModalMods, const FImpactModalObjAssetProxyPtr& ResonModalsParams, const FModalMods& ResonModalMods)
	{
		const float CurrentTime = TimeStep * CurrentFrame;
		bool bCanStillSpawn = false;
		if((SpawnParams.SpawnRate > 0.f) && ((SpawnParams.SpawnDuration <= 0.f) || (CurrentTime < SpawnParams.SpawnDuration)))
		{
			const float SpawnChance = SpawnParams.SpawnChance * FMath::Exp(-SpawnParams.SpawnChanceDecayRate * CurrentTime);
			const float GainMin = SpawnParams.GainMin * FMath::Exp(-SpawnParams.GainDecayRate * CurrentTime);
			if(SpawnChance > 1e-2 && GainMin > 1e-2)
			{
				bCanStillSpawn = true;
				const float DeltaSpawnTime = 1.0f / SpawnParams.SpawnRate;
				if((CurrentFrame == 0) || (CurrentTime - LastImpactSpawnTime + TimeStep > DeltaSpawnTime))
				{
					LastImpactSpawnTime = CurrentTime + TimeStep;
					if((CurrentFrame == 0) || (ImpactRandomStream.FRand() <= SpawnChance))
					{
						FScratchImpactInfo Info;
						const int32 RandFrame = ImpactRandomStream.RandRange(SpawnParams.FrameStartMin, SpawnParams.FrameStartMax);
						Info.Frame = CurrentFrame - FMath::Max(0, RandFrame);
						Info.bResidualEnable = ResidualSynth.IsValid();
						Info.ResidualPitchScale = SpawnParams.ResidualPitchScale;
						Info.bExciteModalEnable = ExciteModalsParams.IsValid();
						Info.bResonModalEnable = ResonModalsParams.IsValid();
						
						Info.ExciteModalMods = ExciteModalMods;
						Info.ResonModalMods = ResonModalMods;

						float Gain = GainMin + SpawnParams.GainRange * ImpactRandomStream.FRand();
						const int32 Count = static_cast<int32>(Info.bResidualEnable) + static_cast<int32>(Info.bExciteModalEnable) + static_cast<int32>(Info.bResonModalEnable);
						Gain = Gain / FMath::Max(1.0f, Count);
						Info.AmpScale = Gain * (1.0 + 0.1f * (ImpactRandomStream.FRand() - 0.5f));
						Info.ExciteModalMods.AmplitudeScale *= Gain * (1.0 + 0.1f * (ImpactRandomStream.FRand() - 0.5f));
						Info.ResonModalMods.AmplitudeScale *= Gain * (1.0 + 0.1f * (ImpactRandomStream.FRand() - 0.5f));
						ImpactInfos.Emplace(Info);
					}
				}
			}
		}
		
		return bCanStillSpawn;
	}

	void FScratchingSynth::SynthesizeCurrentImpacts(const FImpactModalObjAssetProxyPtr& ExciteModalsParams, const FImpactModalObjAssetProxyPtr& ResonModalsParams)
	{
		FMemory::Memzero(MagBuffer.GetData(), MagBuffer.Num() * sizeof(float));
		if(ResidualSynth.IsValid())
		{
			for(int i = 0; i < ImpactInfos.Num(); i++)
			{
				FScratchImpactInfo* Info = &ImpactInfos[i];
				const int32 DeltaFrame = CurrentFrame - ImpactInfos[i].Frame;

				if(Info->bResidualEnable)
					Info->bResidualEnable = ResidualSynth->GetFFTMagAtFrame(DeltaFrame, MagBuffer, Info->AmpScale, Info->ResidualPitchScale);					
				
				if(Info->bExciteModalEnable && ExciteModalsParams.IsValid())
				{
					Info->bExciteModalEnable = !FModalFFT::GetFFTMag(ExciteModalsParams->GetParams(),
																	SamplingRate, NumFFT, HopSize, MagBuffer,
																	DeltaFrame, Info->ExciteModalMods);
				}
				
				if(Info->bResonModalEnable && ResonModalsParams.IsValid())
				{
					Info->bResonModalEnable = !FModalFFT::GetFFTMag(ResonModalsParams->GetParams(),
																   SamplingRate, NumFFT, HopSize, MagBuffer,
																   DeltaFrame, Info->ResonModalMods);
				}
				
				if(i >= MaxNumImpacts)
				{
					ImpactInfos.RemoveAt(0);
					break;
				}
			}

			ResidualSynth->DoIFFT(MagBuffer);
		}
		else
		{
			for(int i = 0; i < ImpactInfos.Num(); i++)
			{
				FScratchImpactInfo* Info = &ImpactInfos[i];
				const int32 DeltaFrame = CurrentFrame - ImpactInfos[i].Frame;

				if(Info->bExciteModalEnable && ExciteModalsParams.IsValid())
				{
					Info->bExciteModalEnable = !FModalFFT::GetFFTMag(ExciteModalsParams->GetParams(),
																	SamplingRate, NumFFT, HopSize, MagBuffer,
																	DeltaFrame, Info->ExciteModalMods);
				}
				
				if(Info->bResonModalEnable && ResonModalsParams.IsValid())
				{
					Info->bResonModalEnable = !FModalFFT::GetFFTMag(ResonModalsParams->GetParams(),
																   SamplingRate, NumFFT, HopSize, MagBuffer,
																   DeltaFrame, Info->ResonModalMods);
				}

				if(i >= MaxNumImpacts)
				{
					ImpactInfos.RemoveAt(0);
					break;
				}
			}

			DoIFFT();
		}

		int i = 0;
		while (i < ImpactInfos.Num())
		{
			const FScratchImpactInfo* Info = &ImpactInfos[i];
			if(!Info->bResidualEnable && !Info->bExciteModalEnable && !Info->bResonModalEnable)
				ImpactInfos.RemoveAt(i);			
			else
				i++;
		}
	}
}
