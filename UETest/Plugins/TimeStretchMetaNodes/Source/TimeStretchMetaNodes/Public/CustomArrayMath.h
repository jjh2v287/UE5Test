// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "DSP/BufferVectorOperations.h"

namespace CustomArrayMath
{
	TIMESTRETCHMETANODES_API void ArraySign(TArrayView<const float> InBuffer, TArrayView<float> OutBuffer);
	TIMESTRETCHMETANODES_API void ArraySignInPlace(TArrayView<float> InBuffer);
}