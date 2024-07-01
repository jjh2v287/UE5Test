// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UResidualObj;

namespace LBSImpactSFXSynth
{
	namespace Editor
	{
		class IMPACTSFXSYNTHEDITOR_API FCustomEditorUtils
		{
		public:
			static void GetNameAffix(const FString& DefaultAffix, const UPackage* Package, FString& PathName);
			static const FSlateBrush& GetSlateBrushSafe(FName InName);
		};

	}
}
