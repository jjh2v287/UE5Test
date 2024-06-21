// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestType.h"

#define LOCTEXT_NAMESPACE "UKMetasound"

namespace Metasound
{
	DEFINE_METASOUND_ENUM_BEGIN(ETestType, FEnumTestTypeType, "TestType")
		DEFINE_METASOUND_ENUM_ENTRY(
			ETestType::Hard, 
			"EDelayFilterType_LowPass_DisplayName", 
			"Low-Pass", 
			"EDelayFilterType_LowPass_TT", 
			"Low-pass filter"),
		DEFINE_METASOUND_ENUM_ENTRY(
			ETestType::Soft, 
			"EDelayFilterType_HighPass_DisplayName", 
			"High-Pass", 
			"EDelayFilterType_HighPass_TT", 
			"High-pass filter"),
	DEFINE_METASOUND_ENUM_END()
}

#undef LOCTEXT_NAMESPACE
