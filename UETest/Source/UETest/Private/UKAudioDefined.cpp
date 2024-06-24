// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAudioDefined.h"

#define LOCTEXT_NAMESPACE "UKMetasound"

namespace Metasound
{
	DEFINE_METASOUND_ENUM_BEGIN(EUKFootStepSoune, FEnumUKFootStepSouneType, "UKFootStepSoune")
		DEFINE_METASOUND_ENUM_ENTRY(
			EUKFootStepSoune::Ground, 
			"EUKFootStepSoune_Ground_DisplayName", 
			"Ground", 
			"EUKFootStep_Ground_Tooltip", 
			"Ground Floor"),
		DEFINE_METASOUND_ENUM_ENTRY(
			EUKFootStepSoune::Glass, 
			"EUKFootStepSoune_Glass_DisplayName", 
			"Glass", 
			"EUKFootStepSoune_Glass_Tooltip", 
			"Glass Floor"),
	DEFINE_METASOUND_ENUM_ENTRY(
			EUKFootStepSoune::Iron, 
			"EUKFootStepSoune_Iron_DisplayName", 
			"Iron", 
			"EUKFootStepSoune_Iron_Tooltip", 
			"Iron Floor"),
	DEFINE_METASOUND_ENUM_END()
}

#undef LOCTEXT_NAMESPACE
