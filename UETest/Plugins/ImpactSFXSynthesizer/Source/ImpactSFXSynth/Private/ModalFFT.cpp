// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ModalFFT.h"

#include "ImpactSFXSynthLog.h"
#include "ModalSynth.h"

namespace LBSImpactSFXSynth
{
	bool FModalFFT::GetFFTMag(TArrayView<const float> ModalsParams, const float SamplingRate, const int32 NumFFT,
							  const int32 HopSize, TArrayView<float> FFTMagBuffer, const int32 CurrentFrame,
							  const FModalMods& InMods)
	{
		const int32 NumStoredModals = ModalsParams.Num() / FModalSynth::NumParamsPerModal;
		const int32 NumModals = InMods.NumModals > 0 ? FMath::Min(NumStoredModals, InMods.NumModals) : NumStoredModals;
		check(NumModals > 0);
		if(NumModals <= 0)
		{
			UE_LOG(LogImpactSFXSynth, Warning, TEXT("FModalFFT::GetFFTMag: the number of modals is zero"));
			return true;
		}
		
		const int32 NumUsedParams = NumModals * FModalSynth::NumParamsPerModal;
		const float Time = CurrentFrame * HopSize / SamplingRate;
		bool bIsDecayToZero = true;
		for(int i = 0; i < NumUsedParams; i++)
		{
			const float Amp = FMath::Clamp(ModalsParams[i] * InMods.AmplitudeScale, 0.f, 1.f);
			i++;
			const float Decay = -Time * FMath::Clamp(ModalsParams[i] * InMods.DecayScale, 0.f, FModalSynth::DecayMax);
			const float TotalAmp = Amp * NumFFT / 4.0f * FMath::Exp(Decay);
			i++;

			if(TotalAmp < 5e-2)
				continue;
			bIsDecayToZero = false;
			
			const float Freq = FMath::Clamp(ModalsParams[i] * InMods.FreqScale, FModalSynth::FMin, FModalSynth::FMax);

			const float BinF =  Freq * NumFFT / SamplingRate;
			const int32 Bin = FMath::FloorToInt32(BinF);
			const int32 StartBin = FMath::Max(0, Bin - NumAdjBin);
			const int32 EndBin = FMath::Min(FFTMagBuffer.Num(), Bin + NumAdjBin + 1);
			for(int j = StartBin; j < EndBin; j++)
			{
				const float Distance = FMath::Abs(BinF - j);
				const float Percent = FMath::Pow(2., -Distance * Distance);
				FFTMagBuffer[j] += TotalAmp * Percent;
			}
		}
		
		return bIsDecayToZero;
	}
}
