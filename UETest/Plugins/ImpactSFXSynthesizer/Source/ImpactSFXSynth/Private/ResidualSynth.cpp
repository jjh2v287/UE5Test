// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ResidualSynth.h"

#include "CustomStatGroup.h"
#include "ImpactSFXSynthLog.h"
#include "ResidualObj.h"
#include "SynthParamPresets.h"
#include "DSP/AudioFFT.h"
#include "DSP/FloatArrayMath.h"
#include "ImpactSFXSynth/Public/Utils.h"

DECLARE_CYCLE_STAT(TEXT("Residual - Synthesize"), STAT_ResidualSynth, STATGROUP_ImpactSFXSynth);


namespace LBSImpactSFXSynth
{
	FResidualSynth::FResidualSynth(const float InSamplingRate, const int32 InNumFramesPerBlock, const int32 InNumOutChannel,
	                               FResidualDataAssetProxyRef ResidualDataAssetProxyRef,
	                               const float InPlaySpeed, const float InAmplitudeScale, const float InPitchScale, const int32 Seed,
	                               const float InStartTime, const float Duration, const bool InIsLoop,
	                               const float InImpactStrengthScale, const float InRandomLoop, const float InDelayTime, const float InRandomMagnitudeScale)
	: SamplingRate(InSamplingRate), NumFramesPerBlock(InNumFramesPerBlock), NumOutChannel(InNumOutChannel)
	, ResidualDataProxy(ResidualDataAssetProxyRef), AmplitudeScale(InAmplitudeScale * InImpactStrengthScale), PitchScale(InPitchScale)
	, RandomMagnitudeScale(InRandomMagnitudeScale)
	{
		checkf(NumOutChannel > 0 && NumOutChannel <= 2, TEXT("Not support number of channels = %d."), NumOutChannel);
		
		AmplitudeScale = FMath::Max(0.f, AmplitudeScale);
		TimeResolution = 1.0f / SamplingRate;
		bIsRandWithSeed = Seed > -1;
		if(bIsRandWithSeed)
			RandomStream = FRandomStream(Seed);
		else
			RandomStream = FRandomStream(FMath::Rand());
		
		const UResidualObj* ResidualObj = ResidualDataProxy->GetResidualData();
		if(ResidualObj == nullptr)
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FResidualSynth::FResidualSynth: ResidualObj is null!"));
			return;
		}

		const int32 NumAnalyzedFrames = ResidualObj->GetNumFrame();
		if(NumAnalyzedFrames < MinNumAnalyzedErbFrames)
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FResidualSynth::FResidualSynth: ResidualObj number of frames are too low!"));
			return;
		}

		ChangePlaySpeed(InPlaySpeed, InStartTime, Duration, InImpactStrengthScale, ResidualObj);

		bIsLooping = InIsLoop;
		RandomLoop = FMath::Max(InRandomLoop, 0.f);
		RandomMagnitudeScale = FMath::Clamp(RandomMagnitudeScale, 0.f, 1.f);
		IniBuffers(ResidualObj);

		Restart(InDelayTime);
	}

	void FResidualSynth::Restart(const float InDelayTime)
	{
		CurrentSynthBufferIndex = 0;
		SetCurrentErbSynthFrame(StartErbSynthFrame);
		CurrentTime = StartTime;
		CurrentState = ESynthesizerState::Init;
		bIsFinalFrameSynth = false;
		LastEnergyFrame = CurrentErbSynthFrame + 1.f;
		CurrentFrameEnergy = 0.f;
		DelayTime = InDelayTime;
	}

	void FResidualSynth::ChangeScalingParams(const float InStartTime, const float InDuration,
											const float InPlaySpeed, const float InAmplitudeScale,
											const float InPitchScale, const float InImpactStrengthScale)
	{
		const UResidualObj* ResidualObj = ResidualDataProxy->GetResidualData();
		if(ResidualObj == nullptr)
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FResidualSynth::FResidualSynth: ResidualObj is null!"));
			return;
		}
		
		ChangePlaySpeed(InPlaySpeed, InStartTime, InDuration, InImpactStrengthScale, ResidualObj);
		
		PitchScale = InPitchScale;
		
		const float OldAmplitudeScale = AmplitudeScale;
		AmplitudeScale = FMath::Max(InAmplitudeScale * InImpactStrengthScale, 0.f);
		
		if(OldAmplitudeScale > 0.f)
		{ 
			const int32 NumFreq = FFTInterpolateBuffer.Num();
			if(IsPitchScale())
				ErbPitchScaleBuffer.SetNumUninitialized(NumFreq);
			
			for(int i = 0; i < NumFreq; i++)
				FFTInterpolateBuffer[i] = AmplitudeScale * FFTInterpolateBuffer[i] / OldAmplitudeScale;
		}
		else //Can't retrieve prev values so we have to re-calculate interpolating buffers again
			InitInterpolateBuffers(ResidualObj);
	}
	
	void FResidualSynth::ChangePlaySpeed(const float InPlaySpeed, const float InStartTime, const float Duration, const float InImpactStrengthScale, const UResidualObj* ResidualObj)
	{
		const float ImpactPlayScale = FMath::Max(0.9f, 1.0f - 0.2f * FMath::LogX(10.f, InImpactStrengthScale));
		PlaySpeed = ImpactPlayScale * InPlaySpeed * ResidualObj->GetSamplingRate() / SamplingRate;
		PlaySpeed = FMath::Max(PlaySpeed, 1e-3f);
		
		const int32 NumAnalyzedFrames = ResidualObj->GetNumFrame();
		const int32 SamplesPerFrame = ResidualObj->GetNumFFT() / 2;
		const float FrameToDurationConv = SamplesPerFrame / (PlaySpeed * SamplingRate);
		const float MaxDuration = NumAnalyzedFrames * FrameToDurationConv;
		const float StartAtFrame = InStartTime * SamplingRate / SamplesPerFrame;
		const float MinNumSynthFrames = FMath::Min(PlaySpeed * MinNumAnalyzedErbFrames, MinNumAnalyzedErbFrames); //Allow finer resolution for PlaySpeed < 1

		StartErbSynthFrame = FMath::Clamp(StartAtFrame * PlaySpeed, -(NumAnalyzedFrames - 1), NumAnalyzedFrames - MinNumSynthFrames);
		StartTime = StartErbSynthFrame * FrameToDurationConv;
		
		if(Duration > 0)
		{
			const float EndErbSynthFrameFloat = Duration / FrameToDurationConv + StartErbSynthFrame;
			const int32 MinEndFrameToSynth = FMath::CeilToInt32(StartErbSynthFrame + MinNumSynthFrames);
			EndErbSynthFrame = FMath::Clamp(FMath::CeilToInt32(EndErbSynthFrameFloat), MinEndFrameToSynth, NumAnalyzedFrames);
			EndTime = FMath::Clamp(StartTime + Duration, MinEndFrameToSynth * FrameToDurationConv,  MaxDuration);
		}
		else
		{
			EndErbSynthFrame = NumAnalyzedFrames;
			EndTime = MaxDuration;
		}
	}

	float FResidualSynth::GetCurrentFrameEnergy()
	{
		if(LastEnergyFrame != CurrentErbSynthFrame)
		{
			LastEnergyFrame = CurrentErbSynthFrame;
			Audio::ArrayMean(ErbFrameBuffer, CurrentFrameEnergy);
		}
		
		return CurrentFrameEnergy;
	}

	bool FResidualSynth::IsFinished() const
	{
		return CurrentState == ESynthesizerState::Finished;
	}

	bool FResidualSynth::IsRunning() const
	{
		return CurrentState == ESynthesizerState::Running || CurrentState == ESynthesizerState::Init;
	}

	void FResidualSynth::ForceStop()
	{
		CurrentState = ESynthesizerState::Finished;
	}

	void FResidualSynth::IniBuffers(const UResidualObj* ResidualObj)
	{
		const int32 NumFFTSynth = ResidualObj->GetNumFFT();
		if(!IsPowerOf2(NumFFTSynth))
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FResidualSynth::FResidualSynth: NumFFT = %d is not a power of 2!"), ResidualObj->GetNumFFT());
			return;
		}
		
		FFFTSettings FFTSettings;
		FFTSettings.Log2Size = Audio::CeilLog2(NumFFTSynth);
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
			GenerateHannWindow(WindowBuffer.GetData(), FFT->NumInputFloats(), true);			
			
			InitInterpolateBuffers(ResidualObj);
		}
	}

	void FResidualSynth::InitInterpolateBuffers(const UResidualObj* ResidualObj)
	{
		const int32 NumErb = ResidualObj->GetNumErb();
		TArray<float> ErbFreqs;
		ErbFreqs.Empty(NumErb);
		const FResidualMagnitudeEffect* MagEffect = ResidualDataProxy->GetMagEffect();
		const float ErbMin = MagEffect->ErbMinShift;
		const float ErbResolution = (MagEffect->ErbMaxShift - ErbMin) / (NumErb - 1);
		for(int i = 0; i < NumErb; i++)
			ErbFreqs.Emplace(UResidualData::Erb2Freq(ErbMin + i * ErbResolution));

		const int32 NumFFTSynth = ResidualObj->GetNumFFT();
		const int32 NumFreq = FFT->NumOutputFloats() / 2;
		FreqResolution = ResidualObj->GetSamplingRate() / NumFFTSynth;
		Freqs.Empty(NumFreq);
		PhaseIndexes.Empty(NumFreq);
		for(int i = 0; i < NumFreq; i++)
		{
			Freqs.Emplace(i * FreqResolution);
			PhaseIndexes.Emplace(RandomStream.RandHelper(FResidualStats::NumSinPoint));
		}
		
		ErbFrameBuffer.SetNumUninitialized(NumFreq);
		
		FFTInterpolateBuffer.SetNumUninitialized(NumFreq);
		FMemory::Memzero(FFTInterpolateBuffer.GetData(), NumFreq * sizeof(float));
		
		FFTInterpolateIdxs.SetNumUninitialized(NumFreq);
		FMemory::Memzero(FFTInterpolateIdxs.GetData(), NumFreq * sizeof(int32));
		
		ErbInterpolateBuffer.SetNumUninitialized(NumErb);
		ErbInterpolateBufferCeil.SetNumUninitialized(NumErb);

		if(IsPitchScale())
			ErbPitchScaleBuffer.SetNumUninitialized(NumFreq);
		
		int32 ErbIdx = 0;
		int32 FreqStartIdx = 0;
		int32 FreqEndIdx = 0;
		const int32 LastBand = NumErb - 1;
		const float NumFFTFloat = NumFFTSynth;
		const int32 ShiftErb = PositiveMod(MagEffect->ShiftByErb, NumErb);
		const int32 ShiftFreq = PositiveMod(MagEffect->ShiftByFreq, NumFreq);
		while(ErbIdx < LastBand && FreqEndIdx < NumFreq)
		{
			while (FreqEndIdx < NumFreq && Freqs[FreqEndIdx] <= ErbFreqs[ErbIdx])
				FreqEndIdx++;

			const int32 NumIdx = FreqEndIdx - FreqStartIdx;
			if(NumIdx > 0)
			{
				const float Energy = AmplitudeScale * NumFFTFloat / NumIdx; //AmplitudeScale is global scale when adding with modal
				const int32 ShiftErbIdx = (ErbIdx + ShiftErb) % NumErb; 
				for(int i = FreqStartIdx; i < FreqEndIdx; i++)
				{
					const int32 ShiftIdx = (i + ShiftFreq) % NumFreq;
					FFTInterpolateBuffer[ShiftIdx] = Energy * ResidualDataProxy->GetFreqScale(Freqs[ShiftIdx]);
					FFTInterpolateIdxs[ShiftIdx] = ShiftErbIdx; //ErbIdx if no shift
				}
			}

			FreqStartIdx = FreqEndIdx;
			ErbIdx++;
		}
		if(FreqEndIdx < NumFreq)
		{ //Can only go in here if while loop is terminated with ErbIdx == LastBand
			const float Energy = AmplitudeScale * NumFFTFloat / (NumFreq - FreqEndIdx);
			const int32 ShiftErbIdx = (ErbIdx + ShiftErb) % NumErb; 
			for(int i = FreqEndIdx; i < NumFreq; i++)
			{
				const int32 ShiftIdx = (i + ShiftFreq) % NumFreq;
				FFTInterpolateBuffer[ShiftIdx] = Energy * ResidualDataProxy->GetFreqScale(Freqs[ShiftIdx]);
				FFTInterpolateIdxs[ShiftIdx] = ShiftErbIdx; //ErbIdx if no shift
			}
		}
	}

	bool FResidualSynth::Synthesize(FMultichannelBufferView& OutAudio, bool bAddToOutput, bool bClampOutput)
	{
		SCOPE_CYCLE_COUNTER(STAT_ResidualSynth);

		if(CurrentState == ESynthesizerState::Finished)
			return true;

		int32 NumRemainOutputFrames = Audio::GetMultichannelBufferNumFrames(OutAudio);
		checkf(NumOutChannel == OutAudio.Num(), TEXT("FResidualSynth::Synthesize: Expect %d channels but requested %d channels"), NumOutChannel, OutAudio.Num());

		if(DelayTime > 0)
		{
			DelayTime -= NumRemainOutputFrames * TimeResolution;
			return false;
		}
		
		const UResidualObj* ResidualObj = ResidualDataProxy->GetResidualData();
		if((NumRemainOutputFrames <= 0) || (ResidualObj == nullptr) || !FFT.IsValid())
		{
			if(ResidualObj)
			{
				UE_LOG(LogImpactSFXSynth, Error, TEXT("FResidualSynth::Synthesize: Unable to synthesize new data due to unexpected errors!"));
			}
			else
			{
				UE_LOG(LogImpactSFXSynth, Error, TEXT("FResidualSynth::Synthesize: Unable to synthesize new data due to ResidualObj = null!"));
			}
				
			return true;
		}
		
		if(CurrentState == ESynthesizerState::Init)
		{   // First Frame so we have to populate OutRealBuffer first
			SynthesizeOneFrame(ResidualObj);
			// Force synthesized another frame after first frame for overlap add
			CurrentSynthBufferIndex = SynthesizedDataBuffers[0].Num();
			CurrentState = ESynthesizerState::Running;
		}

		int32 CurrentOutIdx = 0;
		while (NumRemainOutputFrames > 0)
		{
			checkf(CurrentSynthBufferIndex <= SynthesizedDataBuffers[0].Num(), TEXT("FResidualSynth::Synthesize: CurrentSynthBufferIndex is outside of buffer size!"))
			if(CurrentSynthBufferIndex >= SynthesizedDataBuffers[0].Num())
			{
				if(bIsFinalFrameSynth) //No frame to synth anymore
				{
					CurrentState = ESynthesizerState::Finished;
					break;
				}
				
				const int32 HalfSIze = OutReals[0].Num() / 2;
				for(int32 Channel = 0; Channel < NumOutChannel; Channel++)
				{
					TArrayView<const float> OutLastHalf = TArrayView<const float>(OutReals[Channel]).Slice(HalfSIze, HalfSIze);
					FMemory::Memcpy(SynthesizedDataBuffers[Channel].GetData(), OutLastHalf.GetData(), HalfSIze * sizeof(float));
				}
				
				SynthesizeOneFrame(ResidualObj);
				for(int32 Channel = 0; Channel < NumOutChannel; Channel++)
				{
					const TArrayView<const float> OutFirstHalf = TArrayView<const float>(OutReals[Channel]).Slice(0, HalfSIze);
					Audio::ArrayAddInPlace(OutFirstHalf, SynthesizedDataBuffers[Channel]);
				}
				CurrentSynthBufferIndex = 0;
			}
			
			const int32 NumRemainSynthData = SynthesizedDataBuffers[0].Num() - CurrentSynthBufferIndex;
			const int32 NumCopyFrame = FMath::Min(NumRemainOutputFrames, NumRemainSynthData);
			
			for(int32 Channel = 0; Channel < NumOutChannel; Channel++)
			{
				auto OutDest = OutAudio[Channel].Slice(CurrentOutIdx, NumCopyFrame);
				auto InSource = TArrayView<const float>(SynthesizedDataBuffers[Channel]).Slice(CurrentSynthBufferIndex, NumCopyFrame);
				if(bAddToOutput)
					Audio::ArrayAddInPlace(InSource, OutDest);
				else
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
		
		CurrentTime += CurrentOutIdx * TimeResolution;
		if(bIsLooping && CurrentTime > EndTime)
			CurrentTime = StartTime;
		
		return CurrentState == ESynthesizerState::Finished;
	}

	void FResidualSynth::SynthesizeFull(FResidualSynth& ResidualSynth, FAlignedFloatBuffer& SynthesizedData)
	{
		const UResidualObj* ResidualObj = ResidualSynth.ResidualDataProxy->GetResidualData();
		if(ResidualObj == nullptr)
			return;
		
		const int32 HopSize = ResidualObj->GetNumFFT() / 2;
		int32 NumOutSamples = FMath::CeilToInt32(ResidualSynth.GetMaxDuration() * ResidualSynth.GetSamplingRate());
		NumOutSamples = FMath::Max(HopSize, NumOutSamples);
		SynthesizedData.SetNumUninitialized(NumOutSamples);
		FMemory::Memzero(SynthesizedData.GetData(), NumOutSamples * sizeof(float));
				
		FMultichannelBufferView OutAudioView;
		OutAudioView.Emplace(SynthesizedData);
		int32 CurrentFrame = 0;
		bool bIsFinished = false;
		int32 EndIdx = 0;
		while (!bIsFinished && EndIdx <= NumOutSamples)
		{
			FMultichannelBufferView BufferView = SliceMultichannelBufferView(OutAudioView, CurrentFrame * HopSize, HopSize);
			bIsFinished = ResidualSynth.Synthesize(BufferView, false, true);
			CurrentFrame++;
			EndIdx = CurrentFrame * HopSize + HopSize;
		}
	}

	void FResidualSynth::SetCurrentErbSynthFrame(const float InValue)
	{
		CurrentErbSynthFrame = FMath::Clamp(InValue, StartErbSynthFrame, EndErbSynthFrame-1.f);
	}

	void FResidualSynth::SynthesizeOneFrame(const UResidualObj* ResidualObj)
	{
		GetErbDataByInterpolatingFrames(ResidualObj);
		PutFrameDataToBuffers();
		CalNewPhaseIndex();
		
		Audio::ArrayMultiplyInPlace(FFTInterpolateBuffer, ErbFrameBuffer);
		
		SetConjBuffer();
		
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
		
		const float FinalFrame = EndErbSynthFrame - FMath::Min(1.f, PlaySpeed);
		bIsFinalFrameSynth = CurrentErbSynthFrame >= FinalFrame;

		if(bIsFinalFrameSynth)
		{
			if(bIsLooping)
			{
				bIsFinalFrameSynth = false;
				//Next loop will always start at StartErbSynthFrame to avoid sliding time issues with different PlaySpeed
				//Though this can cause some hiccup in output
				//First frame is warm up frame so need to add PlaySpeed here
				const int32 SamplesPerFrame = ResidualObj->GetNumFFT() / 2;
				const float RandStartFrame = (RandomStream.FRand() * RandomLoop) * SamplingRate * PlaySpeed / SamplesPerFrame;
				SetCurrentErbSynthFrame(StartErbSynthFrame + PlaySpeed + RandStartFrame);
			}
		}
		else
			CurrentErbSynthFrame += PlaySpeed;
	}
	
	bool FResidualSynth::GetFFTMagAtFrame(const int32 NthFrame, TArrayView<float> MagBuffer, const float AmpScale, const float InPitchScale)
	{
		const int32 HopSize = ResidualDataProxy->GetNumFFT() / 2; 
		CurrentErbSynthFrame = StartErbSynthFrame + FMath::Max(0, NthFrame) * PlaySpeed;
		SetCurrentTime(StartTime + NthFrame * HopSize / SamplingRate);
		
		GetErbDataByInterpolatingFrames(ResidualDataProxy->GetResidualData());
		PutFrameDataToMagBuffer();
		
		Audio::ArrayMultiplyInPlace(FFTInterpolateBuffer, ErbFrameBuffer);
		ArrayMultiplyByConstantInPlace(ErbFrameBuffer, AmpScale);
		PitchScale = InPitchScale;
		if(IsPitchScale())
		{
			const float ChirpRate = ResidualDataProxy->GetMagEffect()->ChirpRate * FMath::Abs(CurrentTime);
			
			for(int FreqIdx = 0; FreqIdx < ErbFrameBuffer.Num(); FreqIdx++)
			{
				const float Value = ErbFrameBuffer[FreqIdx];
				const float ScaledIdx = FMath::Max(FreqIdx * PitchScale + ChirpRate, 0.f);
				const int32 Lower = FMath::FloorToInt32(ScaledIdx);
				if(Lower < ErbFrameBuffer.Num())
				{
					const float Percent = ScaledIdx - Lower;
					MagBuffer[Lower] += Value * (1.0f - Percent);
		
					const int32 Upper = Lower + 1;
					if(Upper < ErbFrameBuffer.Num())
						MagBuffer[Upper] += Value * Percent;
				}
			}
		}
		else
			Audio::ArrayAddInPlace(ErbFrameBuffer, MagBuffer);

		const float FinalFrame = EndErbSynthFrame - FMath::Min(1.f, PlaySpeed);
		return  CurrentErbSynthFrame < FinalFrame;
	}

	void FResidualSynth::DoIFFT(TArrayView<const float> MagBuffer)
	{
		const int32 NumPoints = ConjBuffers[0].Num();
		for(int i = 0; i < NumPoints; i++)
		{
			ConjBuffers[0][i] = FResidualStats::SinCycle[(PhaseIndexes[i] + FResidualStats::CosShift) % FResidualStats::NumSinPoint];
			ConjBuffers[1][i] = FResidualStats::SinCycle[PhaseIndexes[i]];
		}
		
		Audio::ArrayMultiplyInPlace(MagBuffer, ConjBuffers[0]);
		Audio::ArrayMultiplyInPlace(MagBuffer, ConjBuffers[1]);

		CalNewPhaseIndex();
		
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

	bool FResidualSynth::IsInFrameRange() const
	{
		return CurrentErbSynthFrame < EndErbSynthFrame;
	}

	void FResidualSynth::GetErbDataByInterpolatingFrames(const UResidualObj* ResidualObj)
	{
		const int32 NumErb = ResidualObj->GetNumErb();
		const TArrayView<const float> ResidualData = ResidualObj->GetDataView();
		const float AbsErbSynthFrame = FMath::Abs(CurrentErbSynthFrame);
		const int32 LastErbSynthFrame = ResidualObj->GetNumFrame() - 1;
		const int32 FloorIdx = FMath::Min(LastErbSynthFrame, FMath::FloorToInt32(AbsErbSynthFrame));
		
		FMemory::Memcpy(ErbInterpolateBuffer.GetData(), ResidualData.Slice(FloorIdx * NumErb, NumErb).GetData(), NumErb * sizeof(float));

		if(!FMath::IsNearlyEqual(AbsErbSynthFrame, FloorIdx, 1e-3f))
		{
			const int32 CeilIdx = FMath::CeilToInt32(AbsErbSynthFrame);
			//CeilIdx - FloorIdx can > 1 at final frame where AbsErbSynthFrame has a much larger value than EndErbSynthFrameInt - 1
			const float InterPercent = (AbsErbSynthFrame - FloorIdx) / (CeilIdx - FloorIdx);
			Audio::ArrayMultiplyByConstantInPlace(ErbInterpolateBuffer, 1.0f - InterPercent);
			
			if(CeilIdx < ResidualObj->GetNumFrame())
				FMemory::Memcpy(ErbInterpolateBufferCeil.GetData(), ResidualData.Slice(CeilIdx * NumErb, NumErb).GetData(), NumErb * sizeof(float));
			else
			{
				if(bIsLooping)
				{
					const int32 StartFrame = FMath::FloorToInt32(FMath::Abs(StartErbSynthFrame)) * NumErb;
					FMemory::Memcpy(ErbInterpolateBufferCeil.GetData(), ResidualData.Slice(StartFrame, NumErb).GetData(), NumErb * sizeof(float));
				}
				else
					FMemory::Memzero(ErbInterpolateBufferCeil.GetData(), ErbInterpolateBufferCeil.Num() * sizeof(float));
			}
			Audio::ArrayMultiplyByConstantInPlace(ErbInterpolateBufferCeil, InterPercent);
			Audio::ArrayAddInPlace(ErbInterpolateBufferCeil, ErbInterpolateBuffer);
		}
	}

	void FResidualSynth::PutFrameDataToBuffers()
	{
		const int32 NumInterPoint = FFTInterpolateIdxs.Num();
		const int32 CircularShift = FMath::RoundToInt32(ResidualDataProxy->GetMagEffect()->CircularShift * CurrentTime);
		if(CircularShift == 0)
		{
			if(ResidualDataProxy->HasAmplitudeScaleModifier())
			{
				if(FMath::IsNearlyZero(RandomMagnitudeScale, 1e-3f))
				{
					for(int i = 0; i < NumInterPoint; i++)
					{
						ErbFrameBuffer[i] = ErbInterpolateBuffer[FFTInterpolateIdxs[i]] *  ResidualDataProxy->GetAmplitudeScale(CurrentTime);
						ConjBuffers[0][i] = FResidualStats::SinCycle[(PhaseIndexes[i] + FResidualStats::CosShift) % FResidualStats::NumSinPoint];
						ConjBuffers[1][i] = FResidualStats::SinCycle[PhaseIndexes[i]];
					}
				}
				else
				{
					const float MinRandScale = 1.f - RandomMagnitudeScale;
					const float RandRange = 2.f * RandomMagnitudeScale;
					for(int i = 0; i < NumInterPoint; i++)
					{
						const float RandScale = GetRandRange(RandomStream, MinRandScale, RandRange); 
						ErbFrameBuffer[i] = ErbInterpolateBuffer[FFTInterpolateIdxs[i]] * ResidualDataProxy->GetAmplitudeScale(CurrentTime) * RandScale;
						ConjBuffers[0][i] = FResidualStats::SinCycle[(PhaseIndexes[i] + FResidualStats::CosShift) % FResidualStats::NumSinPoint];
						ConjBuffers[1][i] = FResidualStats::SinCycle[PhaseIndexes[i]];
					}
				}
			}
			else
			{
				if(FMath::IsNearlyZero(RandomMagnitudeScale, 1e-3f))
				{
					for(int i = 0; i < NumInterPoint; i++)
					{				 
						ErbFrameBuffer[i] = ErbInterpolateBuffer[FFTInterpolateIdxs[i]];
						ConjBuffers[0][i] = FResidualStats::SinCycle[(PhaseIndexes[i] + FResidualStats::CosShift) % FResidualStats::NumSinPoint];
						ConjBuffers[1][i] = FResidualStats::SinCycle[PhaseIndexes[i]];
					}
				}
				else
				{
					const float MinRandScale = 1.f - RandomMagnitudeScale;
					const float RandRange = 2.f * RandomMagnitudeScale;
					for(int i = 0; i < NumInterPoint; i++)
					{
						const float RandScale = GetRandRange(RandomStream, MinRandScale, RandRange); 
						ErbFrameBuffer[i] = ErbInterpolateBuffer[FFTInterpolateIdxs[i]] * RandScale;
						ConjBuffers[0][i] = FResidualStats::SinCycle[(PhaseIndexes[i] + FResidualStats::CosShift) % FResidualStats::NumSinPoint];
						ConjBuffers[1][i] = FResidualStats::SinCycle[PhaseIndexes[i]];
					}
				}
			}
		}
		else
		{
			const int32 NumErb = ResidualDataProxy->GetResidualData()->GetNumErb();
			if(ResidualDataProxy->HasAmplitudeScaleModifier())
			{
				if(FMath::IsNearlyZero(RandomMagnitudeScale, 1e-3f))
				{
					for(int i = 0; i < NumInterPoint; i++)
					{
						ErbFrameBuffer[i] = ErbInterpolateBuffer[PositiveMod(FFTInterpolateIdxs[i] + CircularShift, NumErb)] * ResidualDataProxy->GetAmplitudeScale(CurrentTime);
						ConjBuffers[0][i] = FResidualStats::SinCycle[(PhaseIndexes[i] + FResidualStats::CosShift) % FResidualStats::NumSinPoint];
						ConjBuffers[1][i] = FResidualStats::SinCycle[PhaseIndexes[i]];
					}
				}
				else
				{
					const float MinRandScale = 1.f - RandomMagnitudeScale;
					const float RandRange = 2.f * RandomMagnitudeScale;
					for(int i = 0; i < NumInterPoint; i++)
					{
						const float RandScale = GetRandRange(RandomStream, MinRandScale, RandRange); 
						ErbFrameBuffer[i] = ErbInterpolateBuffer[PositiveMod(FFTInterpolateIdxs[i] + CircularShift, NumErb)] * ResidualDataProxy->GetAmplitudeScale(CurrentTime);
						ErbFrameBuffer[i] *=  RandScale;
						ConjBuffers[0][i] = FResidualStats::SinCycle[(PhaseIndexes[i] + FResidualStats::CosShift) % FResidualStats::NumSinPoint];
						ConjBuffers[1][i] = FResidualStats::SinCycle[PhaseIndexes[i]];
					}
				}
			}
			else
			{
				if(FMath::IsNearlyZero(RandomMagnitudeScale, 1e-3f))
				{
					for(int i = 0; i < NumInterPoint; i++)
					{
						ErbFrameBuffer[i] = ErbInterpolateBuffer[PositiveMod(FFTInterpolateIdxs[i] + CircularShift, NumErb)];
						ConjBuffers[0][i] = FResidualStats::SinCycle[(PhaseIndexes[i] + FResidualStats::CosShift) % FResidualStats::NumSinPoint];
						ConjBuffers[1][i] = FResidualStats::SinCycle[PhaseIndexes[i]];
					}
				}
				else
				{
					const float MinRandScale = 1.f - RandomMagnitudeScale;
					const float RandRange = 2.f * RandomMagnitudeScale;
					for(int i = 0; i < NumInterPoint; i++)
					{
						const float RandScale = GetRandRange(RandomStream, MinRandScale, RandRange); 
						ErbFrameBuffer[i] = ErbInterpolateBuffer[PositiveMod(FFTInterpolateIdxs[i] + CircularShift, NumErb)] * RandScale;
						ConjBuffers[0][i] = FResidualStats::SinCycle[(PhaseIndexes[i] + FResidualStats::CosShift) % FResidualStats::NumSinPoint];
						ConjBuffers[1][i] = FResidualStats::SinCycle[PhaseIndexes[i]];
					}
				}
			}
		}
	}
	
	void FResidualSynth::PutFrameDataToMagBuffer()
	{
		const int32 NumInterPoint = FFTInterpolateIdxs.Num();
		const int32 CircularShift = FMath::RoundToInt32(ResidualDataProxy->GetMagEffect()->CircularShift * CurrentTime);
		if(CircularShift == 0)
		{
			if(ResidualDataProxy->HasAmplitudeScaleModifier())
			{
				if(FMath::IsNearlyZero(RandomMagnitudeScale, 1e-3f))
				{
					for(int i = 0; i < NumInterPoint; i++)
					{
						ErbFrameBuffer[i] = ErbInterpolateBuffer[FFTInterpolateIdxs[i]] *  ResidualDataProxy->GetAmplitudeScale(CurrentTime);
					}
				}
				else
				{
					const float MinRandScale = 1.f - RandomMagnitudeScale;
					const float RandRange = 2.f * RandomMagnitudeScale;
					for(int i = 0; i < NumInterPoint; i++)
					{
						const float RandScale = GetRandRange(RandomStream, MinRandScale, RandRange); 
						ErbFrameBuffer[i] = ErbInterpolateBuffer[FFTInterpolateIdxs[i]] * ResidualDataProxy->GetAmplitudeScale(CurrentTime) * RandScale;
					}
				}
			}
			else
			{
				if(FMath::IsNearlyZero(RandomMagnitudeScale, 1e-3f))
				{
					for(int i = 0; i < NumInterPoint; i++)
					{				 
						ErbFrameBuffer[i] = ErbInterpolateBuffer[FFTInterpolateIdxs[i]];
					}
				}
				else
				{
					const float MinRandScale = 1.f - RandomMagnitudeScale;
					const float RandRange = 2.f * RandomMagnitudeScale;
					for(int i = 0; i < NumInterPoint; i++)
					{
						const float RandScale = GetRandRange(RandomStream, MinRandScale, RandRange); 
						ErbFrameBuffer[i] = ErbInterpolateBuffer[FFTInterpolateIdxs[i]] * RandScale;
					}
				}
			}
		}
		else
		{
			const int32 NumErb = ResidualDataProxy->GetResidualData()->GetNumErb();
			if(ResidualDataProxy->HasAmplitudeScaleModifier())
			{
				if(FMath::IsNearlyZero(RandomMagnitudeScale, 1e-3f))
				{
					for(int i = 0; i < NumInterPoint; i++)
					{
						ErbFrameBuffer[i] = ErbInterpolateBuffer[PositiveMod(FFTInterpolateIdxs[i] + CircularShift, NumErb)] * ResidualDataProxy->GetAmplitudeScale(CurrentTime);
					}
				}
				else
				{
					const float MinRandScale = 1.f - RandomMagnitudeScale;
					const float RandRange = 2.f * RandomMagnitudeScale;
					for(int i = 0; i < NumInterPoint; i++)
					{
						const float RandScale = GetRandRange(RandomStream, MinRandScale, RandRange); 
						ErbFrameBuffer[i] = ErbInterpolateBuffer[PositiveMod(FFTInterpolateIdxs[i] + CircularShift, NumErb)] * ResidualDataProxy->GetAmplitudeScale(CurrentTime);
						ErbFrameBuffer[i] *=  RandScale;
					}
				}
			}
			else
			{
				if(FMath::IsNearlyZero(RandomMagnitudeScale, 1e-3f))
				{
					for(int i = 0; i < NumInterPoint; i++)
					{
						ErbFrameBuffer[i] = ErbInterpolateBuffer[PositiveMod(FFTInterpolateIdxs[i] + CircularShift, NumErb)];
					}
				}
				else
				{
					const float MinRandScale = 1.f - RandomMagnitudeScale;
					const float RandRange = 2.f * RandomMagnitudeScale;
					for(int i = 0; i < NumInterPoint; i++)
					{
						const float RandScale = GetRandRange(RandomStream, MinRandScale, RandRange); 
						ErbFrameBuffer[i] = ErbInterpolateBuffer[PositiveMod(FFTInterpolateIdxs[i] + CircularShift, NumErb)] * RandScale;
					}
				}
			}
		}
	}
	
	void FResidualSynth::CalNewPhaseIndex()
	{
		const int32 NumPhase = PhaseIndexes.Num();
		const FResidualPhaseEffect PhaseEffect = ResidualDataProxy->GetPhaseEffect();
		switch (PhaseEffect.EffectType)
		{
		case EResidualPhaseGenerator::Constant:
			break;
		case EResidualPhaseGenerator::IncrementByFreq:
			{
				const float PhiSpeed = (ResidualDataProxy->GetResidualData()->GetNumFFT() / (SamplingRate * FResidualStats::SinStep)) * UE_PI * PhaseEffect.EffectScale;
				for(int i = 0; i < NumPhase; i++)
				{
					const int32 Idx =  PhaseIndexes[i] + FMath::RoundToInt32(PhiSpeed * Freqs[i]);
					PhaseIndexes[i] = Idx % FResidualStats::NumSinPoint;
				}
			}
			break;
		default:
			{
				for(int i = 0; i < NumPhase; i++)
					PhaseIndexes[i] = RandomStream.RandHelper(FResidualStats::NumSinPoint);
			}
			break;
		}
	}

	void FResidualSynth::SetConjBuffer()
	{
		if(IsPitchScale())
		{
			const float ChirpRate = ResidualDataProxy->GetMagEffect()->ChirpRate * FMath::Abs(CurrentTime);
			FMemory::Memzero(ErbPitchScaleBuffer.GetData(), ErbFrameBuffer.Num() * sizeof(float));
			
			for(int FreqIdx = 0; FreqIdx < ErbFrameBuffer.Num(); FreqIdx++)
			{
				const float Value = ErbFrameBuffer[FreqIdx];
				const float ScaledIdx = FMath::Max(0.f, FreqIdx * PitchScale + ChirpRate);
				const int32 Lower = FMath::FloorToInt32(ScaledIdx);
				if(Lower < ErbFrameBuffer.Num())
				{
					const float Percent = ScaledIdx - Lower;
					ErbPitchScaleBuffer[Lower] += Value * (1.0f - Percent);
		
					const int32 Upper = Lower + 1;
					if(Upper < ErbFrameBuffer.Num())
						ErbPitchScaleBuffer[Upper] += Value * Percent;
				}
			}
			Audio::ArrayMultiplyInPlace(ErbPitchScaleBuffer, ConjBuffers[0]);
			Audio::ArrayMultiplyInPlace(ErbPitchScaleBuffer, ConjBuffers[1]);
		}
		else
		{
			Audio::ArrayMultiplyInPlace(ErbFrameBuffer, ConjBuffers[0]);
			Audio::ArrayMultiplyInPlace(ErbFrameBuffer, ConjBuffers[1]);
		}
	}

	void FResidualSynth::SetCurrentTime(const float InTime)
	{
		CurrentTime = FMath::Clamp(InTime, StartTime, EndTime);
	}
	
	void FResidualSynth::GenerateHannWindow(float* InWindowBuffer, int32 NumFrames, bool bIsPeriodic)
	{
		const int32 N = bIsPeriodic ? NumFrames : NumFrames - 1;
		const float PhaseDelta = 2.0f * PI / N;
		float Phase = 0.0f;

		for (int32 FrameIndex = 0; FrameIndex < NumFrames; FrameIndex++)
		{
			const float Value = 0.5f * (1 - FMath::Cos(Phase));
			Phase += PhaseDelta;
			InWindowBuffer[FrameIndex] = Value;
		}
	}
	
	float FResidualSynth::GetCurrentTIme() const
	{
		return CurrentTime;
	}

	bool FResidualSynth::IsPitchScale() const
	{
		const float ChirpRate = ResidualDataProxy->GetMagEffect()->ChirpRate;
		return !FMath::IsNearlyEqual(PitchScale, 1.0f, 1e-3f) || !FMath::IsNearlyZero(ChirpRate, 1e-2);
	}
}

