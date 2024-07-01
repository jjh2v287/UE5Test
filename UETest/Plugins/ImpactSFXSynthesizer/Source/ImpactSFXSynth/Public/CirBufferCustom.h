// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "DSP/AlignedBuffer.h"

namespace LBSImpactSFXSynth
{
	/** Copy from Epic Implementation then revise to allow random access to current data instead of ReadIndex
	 * ------------
	 * Basic implementation of a circular buffer built for pushing and popping arbitrary amounts of data at once.
	 * Designed to be thread safe for SPSC; However, if Push() and Pop() are both trying to access an overlapping area of the buffer,
	 * One of the calls will be truncated. Thus, it is advised that you use a high enough capacity that the producer and consumer are never in contention.
	 */
	template <typename SampleType, size_t Alignment = 16>
	class TCircularAudioBufferCustom
	{
	private:

		TArray<SampleType, TAlignedHeapAllocator<Alignment>> InternalBuffer;
		uint32 Capacity;
		FThreadSafeCounter ReadCounter;
		FThreadSafeCounter WriteCounter;

	public:
		TCircularAudioBufferCustom()
		{
			SetCapacity(0);
		}

		TCircularAudioBufferCustom(const TCircularAudioBufferCustom<SampleType, Alignment>& InOther)
		{
			*this = InOther;
		}

		TCircularAudioBufferCustom& operator=(const TCircularAudioBufferCustom<SampleType, Alignment>& InOther)
		{
			InternalBuffer = InOther.InternalBuffer;
			Capacity = InOther.Capacity;
			ReadCounter.Set(InOther.ReadCounter.GetValue());
			WriteCounter.Set(InOther.WriteCounter.GetValue());

			return *this;
		}


		TCircularAudioBufferCustom(uint32 InCapacity)
		{
			SetCapacity(InCapacity);
		}

		void Reset(uint32 InCapacity = 0)
		{
			SetCapacity(InCapacity);
		}

		void SetCapacity(uint32 InCapacity)
		{
			checkf(InCapacity < (uint32)TNumericLimits<int32>::Max(), TEXT("Max capacity for this buffer is 2,147,483,647 samples. Otherwise our index arithmetic will not work."));
			Capacity = InCapacity + 1;
			ReadCounter.Set(0);
			WriteCounter.Set(0);
			InternalBuffer.Reset();
			InternalBuffer.AddZeroed(Capacity);
		}

		/** Reserve capacity.
		 *
		 * @param InMinimumCapacity - Minimum capacity of circular buffer.
		 * @param bRetainExistingSamples - If true, existing samples in the buffer will be retained. If false, they are discarded.
		 */
		void Reserve(uint32 InMinimumCapacity, bool bRetainExistingSamples)
		{
			if (Capacity <= InMinimumCapacity)
			{
				uint32 NewCapacity = InMinimumCapacity + 1;

				checkf(NewCapacity < (uint32)TNumericLimits<int32>::Max(), TEXT("Max capacity overflow. Requested %d. Maximum allowed %d"), NewCapacity, TNumericLimits<int32>::Max());

				uint32 NumToAdd = NewCapacity - Capacity;
				InternalBuffer.AddZeroed(NumToAdd);
				Capacity = NewCapacity;
			}

			if (!bRetainExistingSamples)
			{
				ReadCounter.Set(0);
				WriteCounter.Set(0);
			}
		}

		/** Push an array of values into circular buffer. */
		int32 Push(TArrayView<const SampleType> InBuffer)
		{
			return Push(InBuffer.GetData(), InBuffer.Num());
		}

		// Pushes some amount of samples into this circular buffer.
		// Returns the amount of samples written.
		// This can only be used for trivially copyable types.
		int32 Push(const SampleType* InBuffer, uint32 NumSamples)
		{
			SampleType* DestBuffer = InternalBuffer.GetData();
			const uint32 ReadIndex = ReadCounter.GetValue();
			const uint32 WriteIndex = WriteCounter.GetValue();

			int32 NumToCopy = FMath::Min<int32>(NumSamples, Remainder());
			const int32 NumToWrite = FMath::Min<int32>(NumToCopy, Capacity - WriteIndex);

			FMemory::Memcpy(&DestBuffer[WriteIndex], InBuffer, NumToWrite * sizeof(SampleType));
			FMemory::Memcpy(&DestBuffer[0], &InBuffer[NumToWrite], (NumToCopy - NumToWrite) * sizeof(SampleType));

			WriteCounter.Set((WriteIndex + NumToCopy) % Capacity);

			return NumToCopy;
		}

		// Pushes some amount of zeros into the circular buffer.
		// Useful when acting as a blocked, mono/interleaved delay line
		int32 PushZeros(uint32 NumSamplesOfZeros)
		{
			SampleType* DestBuffer = InternalBuffer.GetData();
			const uint32 ReadIndex = ReadCounter.GetValue();
			const uint32 WriteIndex = WriteCounter.GetValue();

			int32 NumToZeroEnd = FMath::Min<int32>(NumSamplesOfZeros, Remainder());
			const int32 NumToZeroBegin = FMath::Min<int32>(NumToZeroEnd, Capacity - WriteIndex);

			FMemory::Memzero(&DestBuffer[WriteIndex], NumToZeroBegin * sizeof(SampleType));
			FMemory::Memzero(&DestBuffer[0], (NumToZeroEnd - NumToZeroBegin) * sizeof(SampleType));

			WriteCounter.Set((WriteIndex + NumToZeroEnd) % Capacity);

			return NumToZeroEnd;
		}

		// Push a single sample onto this buffer.
		// Returns false if the buffer is full.
		bool Push(const SampleType& InElement)
		{
			if (Remainder() == 0)
			{
				return false;
			}
			else
			{
				SampleType* DestBuffer = InternalBuffer.GetData();
				const uint32 ReadIndex = ReadCounter.GetValue();
				const uint32 WriteIndex = WriteCounter.GetValue();

				DestBuffer[WriteIndex] = InElement;

				WriteCounter.Set((WriteIndex + 1) % Capacity);
				return true;
			}
		}

		bool Push(SampleType&& InElement)
		{
			if (Remainder() == 0)
			{
				return false;
			}
			else
			{
				SampleType* DestBuffer = InternalBuffer.GetData();
				const uint32 ReadIndex = ReadCounter.GetValue();
				const uint32 WriteIndex = WriteCounter.GetValue();

				DestBuffer[WriteIndex] = MoveTemp(InElement);

				WriteCounter.Set((WriteIndex + 1) % Capacity);
				return true;
			}
		}

		// Same as Pop(), but does not increment the read counter.
		int32 Peek(SampleType* OutBuffer, uint32 NumSamples) const
		{
			const SampleType* SrcBuffer = InternalBuffer.GetData();
			const uint32 ReadIndex = ReadCounter.GetValue();
			const uint32 WriteIndex = WriteCounter.GetValue();

			const int32 NumToCopy = FMath::Min<int32>(NumSamples, Num());
			const int32 NumRead = FMath::Min<int32>(NumToCopy, Capacity - ReadIndex);
			FMemory::Memcpy(OutBuffer, &SrcBuffer[ReadIndex], NumRead * sizeof(SampleType));
				
			FMemory::Memcpy(&OutBuffer[NumRead], &SrcBuffer[0], (NumToCopy - NumRead) * sizeof(SampleType));

			check(NumSamples < ((uint32)TNumericLimits<int32>::Max()));
			return NumToCopy;
		}

		// Same as Pop(), but does not increment the read counter.
		int32 Peek(SampleType* OutBuffer, uint32 StartOffset,  uint32 NumSamples)
		{
			const SampleType* SrcBuffer = InternalBuffer.GetData();
			const uint32 ReadIndex = ReadCounter.GetValue();
			const uint32 WriteIndex = WriteCounter.GetValue();
			
			uint32 StartIndex = ReadIndex + StartOffset;
			if(StartIndex >= Capacity)
				StartIndex -= Capacity;
			
			const int32 NumToCopy = FMath::Min<int32>(NumSamples, Num() - StartOffset);
			
			const int32 NumRead = FMath::Min<int32>(NumToCopy, Capacity - StartIndex);
			FMemory::Memcpy(OutBuffer, &SrcBuffer[StartIndex], NumRead * sizeof(SampleType));
			
			FMemory::Memcpy(&OutBuffer[NumRead], &SrcBuffer[0], (NumToCopy - NumRead) * sizeof(SampleType));

			check(NumSamples < ((uint32)(TNumericLimits<int32>::Max())));
			return NumToCopy;
		}
		
		// Peeks a single element.
		// returns false if the element is empty.
		bool Peek(SampleType& OutElement) const
		{
			if (Num() == 0)
			{
				return false;
			}
			else
			{
				SampleType* SrcBuffer = InternalBuffer.GetData();
				const uint32 ReadIndex = ReadCounter.GetValue();
				
				OutElement = SrcBuffer[ReadIndex];

				return true;
			}
		}

		// Pops some amount of samples into this circular buffer.
		// Returns the amount of samples read.
		int32 Pop(SampleType* OutBuffer, uint32 NumSamples)
		{
			int32 NumSamplesRead = Peek(OutBuffer, NumSamples);
			check(NumSamples < ((uint32)TNumericLimits<int32>::Max()));

			ReadCounter.Set((ReadCounter.GetValue() + NumSamplesRead) % Capacity);

			return NumSamplesRead;
		}

		// Pops some amount of samples into this circular buffer.
		// Returns the amount of samples read.
		int32 Pop(uint32 NumSamples)
		{
			check(NumSamples < ((uint32)TNumericLimits<int32>::Max()));

			int32 NumSamplesRead = FMath::Min<int32>(NumSamples, Num());

			ReadCounter.Set((ReadCounter.GetValue() + NumSamplesRead) % Capacity);

			return NumSamplesRead;
		}

		// Pops a single element.
		// Will assert if the buffer is empty. Please check Num() > 0 before calling.
		SampleType Pop()
		{
			// Calling this when the buffer is empty is considered a fatal error.
			check(Num() > 0);

			SampleType* SrcBuffer = InternalBuffer.GetData();
			const uint32 ReadIndex = ReadCounter.GetValue();

			SampleType PoppedValue = MoveTempIfPossible(InternalBuffer[ReadIndex]);
			ReadCounter.Set((ReadCounter.GetValue() + 1) % Capacity);
			return PoppedValue;
		}

		// When called, seeks the read or write cursor to only retain either the NumSamples latest data
		// (if bRetainOldestSamples is false) or the NumSamples oldest data (if bRetainOldestSamples is true)
		// in the buffer. Cannot be used to increase the capacity of this buffer.
		void SetNum(uint32 NumSamples, bool bRetainOldestSamples = false)
		{
			check(NumSamples < Capacity);

			if (bRetainOldestSamples)
			{
				WriteCounter.Set((ReadCounter.GetValue() + NumSamples) % Capacity);
			}
			else
			{
				int64 ReadCounterNum = ((int32)WriteCounter.GetValue()) - ((int32) NumSamples);
				if (ReadCounterNum < 0)
				{
					ReadCounterNum = Capacity + ReadCounterNum;
				}

				ReadCounter.Set(ReadCounterNum);
			}
		}

		// Get number of samples that can be popped off of the buffer.
		uint32 Num() const
		{
			const int32 ReadIndex = ReadCounter.GetValue();
			const int32 WriteIndex = WriteCounter.GetValue();

			if (WriteIndex >= ReadIndex)
			{
				return WriteIndex - ReadIndex;
			}
			else
			{
				return Capacity - ReadIndex + WriteIndex;
			}
		}

		// Get the current capacity of the buffer
		uint32 GetCapacity() const
		{
			return Capacity;
		}

		// Get number of samples that can be pushed onto the buffer before it is full.
		uint32 Remainder() const
		{
			const uint32 ReadIndex = ReadCounter.GetValue();
			const uint32 WriteIndex = WriteCounter.GetValue();

			return (Capacity - 1 - WriteIndex + ReadIndex) % Capacity;
		}
	};

	/** A deinterleaved multichannel circular buffer */
	using FMultichannelCircularBufferCustom = TArray<TCircularAudioBufferCustom<float>>;
}
