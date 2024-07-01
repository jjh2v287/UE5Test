// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ModalSynth.h"

#include "CustomStatGroup.h"
#include "ExtendArrayMath.h"
#include "ImpactModalObj.h"
#include "ImpactSFXSynthLog.h"
#include "DSP/FloatArrayMath.h"

DECLARE_CYCLE_STAT(TEXT("Modal - Synthesize"), STAT_ModalSynth, STATGROUP_ImpactSFXSynth);

namespace LBSImpactSFXSynth
{
	FModalSynth::FModalSynth(float InSamplingRate, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr,
							const float InStartTime, const float InDuration, const int32 NumUsedModals,
							const float AmplitudeScale, const float DecayScale, const float FreqScale,
							const float InImpactStrengthScale, const float InDampingRatio, const float InDelayTime,
							const bool bRandomlyGetModal)
	: SamplingRate(InSamplingRate), MaxDuration(InDuration)
	{
		if(!ModalsParamsPtr.IsValid())
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FModalSynth:: invalid modal pointer!"));
			return;
		}
		
		InitBuffers(ModalsParamsPtr, NumUsedModals);
		
		ResetAllStates(ModalsParamsPtr, InStartTime, InDuration, AmplitudeScale, DecayScale,
							  FreqScale, InImpactStrengthScale, InDampingRatio, bRandomlyGetModal, InDelayTime);
	}

	bool FModalSynth::IsRunning() const
	{
		return CurrentState == ESynthesizerState::Running || CurrentState == ESynthesizerState::Init; 
	}

	void FModalSynth::ForceStop()
	{
		CurrentState = ESynthesizerState::Finished;
	}

	void FModalSynth::ResetAllStates(const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, 
										 const float InStartTime, const float InDuration,
										 const float AmplitudeScale, const float DecayScale, const float FreqScale,
										 const float InImpactStrengthScale, const float InDampingRatio, const bool bRandomlyGetModal,
										 const float InDelayTime)
	{
		StartTime = InStartTime;
		CurrentTime = StartTime;
		MaxDuration = InDuration;
		CurrentState = ESynthesizerState::Init;
		DelayTime = InDelayTime;
		CurrentTotalAmplitude = 0.f;
		
		CurrentAmpScale = AmplitudeScale;
		CurrentFreqScale = FreqScale;
		
		float DecayImpact = FMath::Lerp(0.5f, 0.f, InDampingRatio);
		DecayImpact = 1.f - DecayImpact * FMath::LogX(10.f,InImpactStrengthScale);
		DecayImpact = FMath::Max(0.01f, DecayImpact);
		CurrentDecayScale = DecayScale * DecayImpact;
		
		TArrayView<const float> ModalsParams = ModalsParamsPtr->GetParams();
		checkf(NumUsedParams <= ModalsParams.Num(), TEXT("FModalSynth::ChangeScalingParams: Number of modal params is changed after initialization!"));

		//ImgBuffer is always init at zero
		FMemory::Memzero(ImgBuffer.GetData(), ImgBuffer.Num() * sizeof(float));
		
		TotalMaxAmplitude = 0.f;
		if(bRandomlyGetModal)
		{
			TArray<int32> ModalIndexes;
			ModalIndexes.Empty(NumTrueModal);
			const int32 LastModalIdx = NumTrueModal - 1;
			const int32 StartIdx = FMath::RandRange(0, LastModalIdx);
			const int32 NumTotalModals = ModalsParamsPtr->GetNumModals();
			const int32 Step = NumTotalModals / NumTrueModal;
			for(int i = 0; i < NumTrueModal; i++)
			{
				const int32 NewIdx = (StartIdx + i * Step) % NumTotalModals;
				ModalIndexes.Emplace(NewIdx);
			}
			
			for(int j = 0; j < NumTrueModal; j++)
			{
				int32 ModalIndex = ModalIndexes[j] * NumParamsPerModal;

				const float Gain = FMath::Clamp(ModalsParams[ModalIndex] * CurrentAmpScale, -1.f, 1.f) * InImpactStrengthScale;
				TotalMaxAmplitude += FMath::Abs(Gain);
				ModalIndex++;

				const float Decay = FMath::Clamp(ModalsParams[ModalIndex] * CurrentDecayScale, DecayMin, DecayMax);
				ModalIndex++;

				RealBuffer[j] =  Gain * FMath::Exp(-Decay * FMath::Abs(StartTime));
				
				const float Freq = UE_TWO_PI *  FMath::Clamp(ModalsParams[ModalIndex] * CurrentFreqScale, FMin, FMax) / SamplingRate;
				const float DecayPerSampling = StartTime < 0.f ? FMath::Exp(Decay / SamplingRate) : FMath::Exp(-Decay / SamplingRate);
				PBuffer[j] = FMath::Cos(Freq) * DecayPerSampling;
				QBuffer[j] = FMath::Sin(Freq) * DecayPerSampling;
			}
		}
		else
		{
			for(int i = 0, j = 0; i < NumUsedParams; i++, j++)
			{
				const float Gain = FMath::Clamp(ModalsParams[i] * CurrentAmpScale, -1.f, 1.f) * InImpactStrengthScale;
				TotalMaxAmplitude += FMath::Abs(Gain);
				i++;

				const float Decay = FMath::Clamp(ModalsParams[i] * CurrentDecayScale, DecayMin, DecayMax);
				i++;
				
				RealBuffer[j] = Gain * FMath::Exp(-Decay * FMath::Abs(StartTime));
				
				const float Freq = UE_TWO_PI *  FMath::Clamp(ModalsParams[i] * CurrentFreqScale, FMin, FMax) / SamplingRate;
				const float DecayPerSampling = StartTime < 0.f ? FMath::Exp(Decay / SamplingRate) : FMath::Exp(-Decay / SamplingRate);
				PBuffer[j] = FMath::Cos(Freq) * DecayPerSampling;
				QBuffer[j] = FMath::Sin(Freq) * DecayPerSampling;
			}
		}
	}
	
	void FModalSynth::InitBuffers(const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, const int32 NumUsedModals)
	{
		const int32 NumTotalParams = ModalsParamsPtr->GetNumParams();
		const int32 NumTotalModals = NumTotalParams / NumParamsPerModal;
		if(NumTotalModals * NumParamsPerModal != NumTotalParams)
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FModalSynth::InitBuffers: Number of input parameters (%d) is incorrect!"), NumTotalParams);
			return;
		}
		
		TimeResolution = 1.0f / SamplingRate;
		int32 NumModals = NumUsedModals > 0 ? FMath::Min(NumTotalModals, NumUsedModals) : NumTotalModals;
		NumTrueModal = NumModals;
		checkf(NumModals > 0, TEXT("FModalSynth::InitBuffers: NumNodals must be larger than 0!"));

		NumUsedParams = NumParamsPerModal * NumModals;

		//Make sure NumModals is a multiplier of Vector register so all modals can run in SIMD when synthesizing
		const int32 RemVec = NumModals % AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER;
		if(RemVec > 0)
			NumModals = NumModals + (AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER - RemVec);
			
		const int32 ClearSize = NumModals * sizeof(float);
		
		ImgBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(ImgBuffer.GetData(), ClearSize);

		RealBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(RealBuffer.GetData(), ClearSize);
		
		QBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(QBuffer.GetData(), ClearSize);
		
		PBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(PBuffer.GetData(), ClearSize);
	}

	bool FModalSynth::Synthesize(FMultichannelBufferView& OutAudio, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr,
								 bool bAddToOutput, bool bClampOutput)
	{
		SCOPE_CYCLE_COUNTER(STAT_ModalSynth);

		if(CurrentState == ESynthesizerState::Finished)
			return true;
		
		if(MaxDuration > 0 && GetDuration() > MaxDuration)
		{
			CurrentState = ESynthesizerState::Finished;
			return true;
		}		
		
		const int32 NumOutputFrames = Audio::GetMultichannelBufferNumFrames(OutAudio);
		FrameTime = NumOutputFrames * TimeResolution;
		if(DelayTime > 0)
		{
			DelayTime -= FrameTime;
			return false;
		}

		TArrayView<const float> ModalParams = ModalsParamsPtr->GetParams();
		if(NumUsedParams > ModalParams.Num())
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FModalSynth::Synthesize: Number of modal parameters (%d) is changed after initialization!"), NumUsedParams);
			return true;
		}
		
		TArrayView<float> FirstChannelBuffer = TArrayView<float>(OutAudio[0].GetData(), NumOutputFrames);
		
		if(StartTime < 0.f && CurrentTime >= 0.f)
		{
			TArrayView<const float> ModalsParams = ModalsParamsPtr->GetParams();
			for(int i = DecayBin, j = 0; i < NumUsedParams && j < NumTrueModal; i += NumParamsPerModal, j++)
			{
				const float Decay = FMath::Clamp(ModalsParams[i] * CurrentDecayScale, DecayMin, DecayMax);
				const float Freq = UE_TWO_PI *  FMath::Clamp(ModalsParams[i + 1] * CurrentFreqScale, FMin, FMax) / SamplingRate;
				const float DecayPerSampling = FMath::Exp(-Decay / SamplingRate);
				PBuffer[j] = FMath::Cos(Freq) * DecayPerSampling;
				QBuffer[j] = FMath::Sin(Freq) * DecayPerSampling;
			}
		}
		
		ExtendArrayMath::ArrayImpactModalEuler(RealBuffer, ImgBuffer, PBuffer, QBuffer, NumTrueModal, FirstChannelBuffer);
		
		CurrentTotalAmplitude = ExtendArrayMath::ArrayModalTotalGain(RealBuffer, ImgBuffer, NumTrueModal);
		
		if(bClampOutput)
			Audio::ArrayClampInPlace(FirstChannelBuffer, -1.f, 1.f);
		
		const int32 NumChannels = OutAudio.Num();
		const float* CurrentData = FirstChannelBuffer.GetData();
		for(int i = 1; i < NumChannels; i++)
		{
			if(bAddToOutput)
				Audio::ArrayAddInPlace(FirstChannelBuffer, OutAudio[i]);
			else
				FMemory::Memcpy(OutAudio[i].GetData(), CurrentData, NumOutputFrames * sizeof(float));
		}

		CurrentTime += FrameTime;
		//Only stop if CurrentTime > 0 as large negative CurrentTime might not be synthesized 
		CurrentState = (CurrentTotalAmplitude < 1e-3f && CurrentTime > 0.f) ? ESynthesizerState::Finished : ESynthesizerState::Running;
		return CurrentState == ESynthesizerState::Finished;
	}

	float FModalSynth::GetCurrentMaxAmplitude() const
	{
		if(CurrentTime == StartTime || CurrentTime <= 0.f)
			return TotalMaxAmplitude;
		
		return CurrentTotalAmplitude;
	}

	void FModalSynth::SynthesizeFullOneChannel(FModalSynth& ModalSynth, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, TArray<float>& SynthData)
	{
		SynthData.Empty();
		
		const int32 NumFrames = static_cast<int32>(ModalSynth.SamplingRate * 0.1f);
		FAlignedFloatBuffer WorkBuffer;
		WorkBuffer.SetNumUninitialized(NumFrames);
		
		FMultichannelBufferView OutAudioView;
		OutAudioView.Emplace(WorkBuffer);
		int32 CurrentIndex = 0;
		bool IsStillRunning = true;
		while (IsStillRunning)
		{
			FMemory::Memzero(WorkBuffer.GetData(), WorkBuffer.Num() * sizeof(float));
			IsStillRunning = !ModalSynth.Synthesize(OutAudioView, ModalsParamsPtr, false, true);
			
			SynthData.AddUninitialized(NumFrames);			
			float* Data = SynthData.GetData();
			FMemory::Memcpy(&Data[CurrentIndex], WorkBuffer.GetData(), NumFrames * sizeof(float));
			CurrentIndex += NumFrames;
		}
	}
}

