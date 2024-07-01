// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OctoMap/OctTreeChild.h"

namespace OctoMap
{
	class IMPACTSFXSYNTH_API FOctTreeRoot
	{
	public:				
		FOctTreeRoot(const FVector& InOrigin, uint8 InMaxDepth, float InSize);
		virtual ~FOctTreeRoot() = default;
		
		bool UpdateOccupancy(const FVector& Location, bool bIsFree);

		void GetAllOccupiedLocation(TArray<FOctBox>& OutArray);
		
	private:
		FVector Origin;
		uint8 MaxDepth;
		float ChildSize;
		uint16 ChildOccupancy;
		TArray<TSharedPtr<FOctTreeChild>> ChildNodes;
	};
}