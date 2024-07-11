// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "MultiChannelBufferCustom.h"

namespace MultiChannelBufferCustom
{
	void SetMultichannelCircularBufferCapacity(int32 InNumChannels, int32 InNumFrames, FMultichannelCircularBufferRand& OutBuffer)
	{
		OutBuffer.SetNum(InNumChannels, false /* bAllowShrinking */);
		for (TCircularAudioBufferCustom<float>& Buffer : OutBuffer)
		{
			Buffer.SetCapacity(InNumFrames);
		}
	}

	int32 GetMultichannelBufferNumFrames(const FMultichannelCircularBufferRand& InBuffer)
	{	
		return PrivateUtil::GetMultichannelBufferNumFrames(InBuffer);
	}

	int32 GetMultichannelBufferNumFrames(const Audio::FMultichannelBuffer& InBuffer)
	{
		return PrivateUtil::GetMultichannelBufferNumFrames(InBuffer);
	}

	int32 GetMultichannelBufferNumFrames(const Audio::FMultichannelBufferView& InBuffer)
	{
		return PrivateUtil::GetMultichannelBufferNumFrames(InBuffer);
	}
}
