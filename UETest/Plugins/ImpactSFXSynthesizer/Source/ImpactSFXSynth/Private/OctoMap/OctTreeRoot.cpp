// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "OctoMap/OctTreeRoot.h"
#include "ImpactSFXSynthLog.h"

namespace OctoMap
{
	FOctTreeRoot::FOctTreeRoot(const FVector& InOrigin, uint8 InMaxDepth, float InSize)
		: Origin(InOrigin)
	{
		MaxDepth = FMath::Min<uint8>(6, InMaxDepth);
		ChildSize = FMath::Max(1e-5f, InSize / 2.0f);
		ChildOccupancy = 0;
	}

	bool FOctTreeRoot::UpdateOccupancy(const FVector& Location, bool bIsFree)
	{
		//Floating point is good enough for delta vector and subsequent child localization
		const FVector3f Delta = FVector3f(Location - Origin);
		if(Delta.GetAbsMax() > ChildSize)
			return false;
		
		if(ChildNodes.Num() == 0)
		{
			ChildNodes.Reserve(FOctTreeChild::NumChild);
			for(int i = 0; i < FOctTreeChild::NumChild; i++)
				ChildNodes.Emplace(MakeShared<FOctTreeChild>());			
		}
		
		uint32 Index;
		FVector3f ChildOrigin;
		const float HalfChildSize = ChildSize / 2.0f;
		FOctTreeChild::GetChildFromDelta(Delta, HalfChildSize, Index, ChildOrigin);
		ChildNodes[Index]->UpdateOccupancy(Delta - ChildOrigin, HalfChildSize, bIsFree, 1, MaxDepth);

		return true;
	}

	void FOctTreeRoot::GetAllOccupiedLocation(TArray<FOctBox>& OutArray)
	{
		if(ChildNodes.Num() == 0)
		{
			if(ChildOccupancy == static_cast<uint16>(EOctChildMask::AllOccupied))
				OutArray.Emplace(FOctBox(Origin, FVector(ChildSize)));
			return;			
		}

		const float HalfChildSize = ChildSize / 2.0f;
		for(int i = 0; i < ChildNodes.Num(); i++)
		{
			FVector ChildOrigin = Origin + static_cast<FVector>(FOctTreeChild::ChildOriginMap[i]) * HalfChildSize;
			ChildNodes[i]->GetAllOccupiedLocation(OutArray, ChildOrigin, ChildSize);
		}
	}
}
