// Georgy Treshchev 2024.

#include "UKOnsetDetection.h"
#include "Math/UnrealMathUtility.h"

UUKOnsetDetection::UUKOnsetDetection()
	: FrameSize(0)
{
}

void UUKOnsetDetection::UpdateFrameSize(int32 InFrameSize)
{
	UpdateFrameSize(static_cast<int64>(InFrameSize));
}

void UUKOnsetDetection::UpdateFrameSize(int64 InFrameSize)
{
	FrameSize = InFrameSize;

	PrevMagnitudeSpectrum_SpectralDifference.SetNum(FrameSize);
	PrevMagnitudeSpectrum_SpectralDifferenceHWR.SetNum(FrameSize);
	PrevPhaseSpectrum_ComplexSpectralDifference.SetNum(FrameSize);
	PrevPhaseSpectrum2_ComplexSpectralDifference.SetNum(FrameSize);
	PrevMagnitudeSpectrum_ComplexSpectralDifference.SetNum(FrameSize);

	PreviousEnergySum = 0;
}

float UUKOnsetDetection::GetEnergyEnvelope(const TArray<float>& AudioFrames)
{
	return GetEnergyEnvelope(TArray64<float>(AudioFrames));
}

float UUKOnsetDetection::GetEnergyEnvelope(const TArray64<float>& AudioFrames)
{
	float EnergyEnvelopeValue{0};

	// Sum the squares of the samples
	for (const auto& BufferValue : AudioFrames)
	{
		EnergyEnvelopeValue += FMath::Pow(BufferValue, 2);
	}

	return EnergyEnvelopeValue;
}

float UUKOnsetDetection::GetEnergyDifference(const TArray<float>& AudioFrames)
{
	return GetEnergyDifference(TArray64<float>(AudioFrames));
}

float UUKOnsetDetection::GetEnergyDifference(const TArray64<float>& AudioFrames)
{
	float EnergyDifferenceValue{0};

	// Sum the squares of the samples
	for (const auto& BufferValue : AudioFrames)
	{
		EnergyDifferenceValue += FMath::Pow(BufferValue, 2);
	}

	// Sample is first order difference in energy
	const float Difference{EnergyDifferenceValue - PreviousEnergySum};

	// Store energy value for next calculation
	PreviousEnergySum = EnergyDifferenceValue;

	if (Difference > 0)
	{
		return Difference;
	}

	return 0.0;
}

float UUKOnsetDetection::GetSpectralDifference(const TArray<float>& MagnitudeSpectrum)
{
	return GetSpectralDifference(TArray64<float>(MagnitudeSpectrum));
}

float UUKOnsetDetection::GetSpectralDifference(const TArray64<float>& MagnitudeSpectrum)
{
	if (MagnitudeSpectrum.Num() != FrameSize)
	{
		UpdateFrameSize(MagnitudeSpectrum.Num());
	}

	float SpectralDifferenceValue{0};

	for (TArray64<float>::SizeType Index = 0; Index < MagnitudeSpectrum.Num(); ++Index)
	{
		// Calculate difference
		const float Difference{MagnitudeSpectrum[Index] - PrevMagnitudeSpectrum_SpectralDifference[Index]};

		// Ensure all difference values are positive
		FMath::Abs(Difference);

		// Add difference to sum
		SpectralDifferenceValue += Difference;

		// Store the sample for next time
		PrevMagnitudeSpectrum_SpectralDifference[Index] = MagnitudeSpectrum[Index];
	}

	return SpectralDifferenceValue;
}

float UUKOnsetDetection::GetSpectralDifferenceHWR(const TArray<float>& MagnitudeSpectrum)
{
	return GetSpectralDifferenceHWR(TArray64<float>(MagnitudeSpectrum));
}

float UUKOnsetDetection::GetSpectralDifferenceHWR(const TArray64<float>& MagnitudeSpectrum)
{
	if (MagnitudeSpectrum.Num() != FrameSize)
	{
		UpdateFrameSize(MagnitudeSpectrum.Num());
	}

	float SpectralDifferenceHWRValue{0};

	for (TArray64<float>::SizeType Index = 0; Index < MagnitudeSpectrum.Num(); ++Index)
	{
		// Calculate difference
		const float Difference = MagnitudeSpectrum[Index] - PrevMagnitudeSpectrum_SpectralDifferenceHWR[Index];

		// Only for positive changes
		if (Difference > 0)
		{
			// Add difference to sum
			SpectralDifferenceHWRValue += Difference;
		}

		// Store the sample for next time
		PrevMagnitudeSpectrum_SpectralDifferenceHWR[Index] = MagnitudeSpectrum[Index];
	}

	return SpectralDifferenceHWRValue;
}

float UUKOnsetDetection::GetComplexSpectralDifference(const TArray<float>& FFTReal, const TArray<float>& FFTImaginary)
{
	return GetComplexSpectralDifference(TArray64<float>(FFTReal), TArray64<float>(FFTImaginary));
}

float UUKOnsetDetection::GetComplexSpectralDifference(const TArray64<float>& FFTReal, const TArray64<float>& FFTImaginary)
{
	if (FFTReal.Num() != FFTImaginary.Num())
	{
		return -1;
	}

	if (FFTReal.Num() != FrameSize)
	{
		UpdateFrameSize(FFTReal.Num());
	}

	float ComplexSpectralDifferenceValue{0};

	// Compute phase values from fft output and sum deviations
	for (TArray64<float>::SizeType Index = 0; Index < FFTReal.Num(); ++Index)
	{
		// Calculate phase value
		const float PhaseValue{FMath::Atan2(FFTImaginary[Index], FFTReal[Index])};

		// Calculate magnitude value
		const float MagnitudeValue{FMath::Sqrt(FMath::Pow(FFTReal[Index], 2) + FMath::Pow(FFTImaginary[Index], 2))};

		// Phase deviation
		const float PhaseDeviation{PhaseValue - (2 * PrevPhaseSpectrum_ComplexSpectralDifference[Index]) + PrevPhaseSpectrum2_ComplexSpectralDifference[Index]};

		// Wrap into [-pi,pi] range
		const float PhasePiRange{Princarg(PhaseDeviation)};

		// Calculate magnitude difference (real part of Euclidean distance between complex frames)
		const float MagnitudeDifference{MagnitudeValue - PrevMagnitudeSpectrum_ComplexSpectralDifference[Index]};

		// Calculate phase difference (imaginary part of Euclidean distance between complex frames)
		const float PhaseDifference{-MagnitudeValue * FMath::Sin(PhasePiRange)};

		// Square real and imaginary parts, sum and take square root
		const float Value{FMath::Sqrt(FMath::Pow(MagnitudeDifference, 2) + FMath::Pow(PhaseDifference, 2))};

		ComplexSpectralDifferenceValue += Value;

		// Store values for the next calculation
		PrevPhaseSpectrum2_ComplexSpectralDifference[Index] = PrevPhaseSpectrum_ComplexSpectralDifference[Index];
		PrevPhaseSpectrum_ComplexSpectralDifference[Index] = PhaseValue;
		PrevMagnitudeSpectrum_ComplexSpectralDifference[Index] = MagnitudeValue;
	}

	return ComplexSpectralDifferenceValue;
}

float UUKOnsetDetection::GetHighFrequencyContent(const TArray<float>& MagnitudeSpectrum)
{
	return GetHighFrequencyContent(TArray64<float>(MagnitudeSpectrum));
}

float UUKOnsetDetection::GetHighFrequencyContent(const TArray64<float>& MagnitudeSpectrum)
{
	float HighFrequencyContentValue{0};

	for (TArray64<float>::SizeType Index = 0; Index < MagnitudeSpectrum.Num(); ++Index)
	{
		HighFrequencyContentValue += MagnitudeSpectrum[Index] * static_cast<float>(Index + 1);
	}

	return HighFrequencyContentValue;
}

void UUKOnsetDetection::ComputeFFT(const TArray<float>& Signal, TArray<float>& RealResult, TArray<float>& ImaginaryResult)
{
	int32 N = Signal.Num();
	RealResult.SetNum(N);
	ImaginaryResult.SetNum(N);

	for (int32 k = 0; k < N; ++k)
	{
		float RealSum = 0.0;
		float ImaginarySum = 0.0;
		for (int n = 0; n < N; ++n)
		{
			float Angle = -2 * UE_PI * k * n / N;
			RealSum += Signal[n] * FMath::Cos(Angle);
			ImaginarySum += Signal[n] * FMath::Sin(Angle);
		}
		RealResult[k] = RealSum;
		ImaginaryResult[k] = ImaginarySum;
	}
}

void UUKOnsetDetection::Reset()
{
	FrameSize = 0;
	PreviousEnergySum = 0;
	PrevMagnitudeSpectrum_SpectralDifference.Reset();
	PrevMagnitudeSpectrum_SpectralDifferenceHWR.Reset();
	PrevPhaseSpectrum_ComplexSpectralDifference.Reset();
	PrevPhaseSpectrum2_ComplexSpectralDifference.Reset();
	PrevMagnitudeSpectrum_ComplexSpectralDifference.Reset();
}