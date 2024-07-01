// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "OctoMap/OctTreeChild.h"
#include "ImpactSFXSynthLog.h"

namespace OctoMap
{
	TArray<FInt32Vector> FOctTreeChild::ChildOriginMap { FInt32Vector(-1, -1, -1), FInt32Vector(1, -1, -1),
														   FInt32Vector(-1, 1, -1), FInt32Vector(1, 1, -1),
														   FInt32Vector(-1, -1, 1), FInt32Vector(1, -1, 1),
														   FInt32Vector(-1, 1, 1), FInt32Vector(1, 1, 1),
														};
	
	FOctTreeChild::FOctTreeChild()
	{
		ChildOccupancy = 0;
	}

	bool FOctTreeChild::UpdateOccupancy(const FVector3f& DeltaVector, float HalfChildSize, uint16 Free, int32 CurrentDepth, int32 MaxDepth)
	{
		uint32 Index;
		FVector3f Origin;
		HalfChildSize = HalfChildSize / 2.0f;
		GetChildFromDelta(DeltaVector, HalfChildSize, Index, Origin);

		//If the state is the same (free or occupied only for leafs nodes), no need to update
		const uint16 Mask = GetFreeOrOccupiedMask(Free);
		if(IsChildMatch(ChildOccupancy, Index, Mask))
			return IsCanBePruned();
		
		if(CurrentDepth == MaxDepth)
		{
			SetChildOccupancy(ChildOccupancy, Index, Mask);
			return IsCanBePruned();
		}

		if(ChildNodes.Num() == 0)
		{
			if(Free == static_cast<uint16>(EOctChildMask::Free) && (ChildOccupancy == 0 || IsAllFreeOrUnknown()))
			{
				SetChildOccupancy(ChildOccupancy, Index, Mask);
				return IsCanBePruned();
			}
			
			ChildNodes.Reserve(NumChild);
			for(int i = 0; i < NumChild; i++)
				ChildNodes.Emplace(MakeShared<FOctTreeChild>());
		}
		
		const bool bCanBePruned = ChildNodes[Index]->UpdateOccupancy(DeltaVector - Origin, HalfChildSize, Free, CurrentDepth + 1, MaxDepth);
		if(bCanBePruned)
		{
			SetChildOccupancy(ChildOccupancy, Index, GetFreeOrOccupiedMask(Free));
			if(IsCanBePruned())
			{
				ChildNodes.Empty();
				return true;
			}
			return false;
		}

		SetChildOccupancy(ChildOccupancy, Index, static_cast<uint16>(EOctChildMask::Inner));
		return false;
	}

	void FOctTreeChild::GetAllOccupiedLocation(TArray<FOctBox>& OutArray, const FVector& ChildOrigin, float ChildSize)
	{
		const float HalfSize = ChildSize / 2.0f;
		if(ChildNodes.Num() == 0)
		{
			if(ChildOccupancy == static_cast<uint16>(EOctChildMask::AllOccupied))
				OutArray.Emplace(FOctBox(ChildOrigin, FVector(HalfSize)));
			else if(ChildOccupancy > 0 && ChildOccupancy != static_cast<uint16>(EOctChildMask::AllFree))
			{
				const float LeafHalfSize = HalfSize / 2.0f;
				for(uint32 i = 0; i < NumChild; i++)
				{
					if(IsChildMatch(ChildOccupancy, i, static_cast<uint16>(EOctChildMask::Occupied)))
					{
						FVector LeafOrigin = ChildOrigin + static_cast<FVector>(ChildOriginMap[i]) * LeafHalfSize;
						OutArray.Emplace(FOctBox(LeafOrigin, FVector(LeafHalfSize)));
					}
				}
			}
			return;			
		}

		const float ChildHalfSize = HalfSize / 2.0f;
		for(int i = 0; i < ChildNodes.Num(); i++)
		{
			FVector NewOrigin = ChildOrigin + static_cast<FVector>(ChildOriginMap[i]) * ChildHalfSize;
			ChildNodes[i]->GetAllOccupiedLocation(OutArray, NewOrigin, HalfSize);
		}
	}

	uint16 FOctTreeChild::GetFreeOrOccupiedMask(uint16 Free)
	{
		return 1 + Free;
	}
	
	bool FOctTreeChild::IsCanBePruned() const
	{
		return (ChildOccupancy == static_cast<uint16>(EOctChildMask::AllFree)
			   || ChildOccupancy == static_cast<uint16>(EOctChildMask::AllOccupied));
	}

	bool FOctTreeChild::IsAllFreeOrUnknown() const
	{
		bool isFree = (ChildOccupancy & 3) < 2;
		isFree &= (((ChildOccupancy >> 2) & 3) < 2);
		isFree &= (((ChildOccupancy >> 4) & 3) < 2);
		isFree &= (((ChildOccupancy >> 6) & 3) < 2);
		isFree &= (((ChildOccupancy >> 8) & 3) < 2);
		isFree &= (((ChildOccupancy >> 10) & 3) < 2);
		isFree &= (((ChildOccupancy >> 12) & 3) < 2);
		isFree &= (((ChildOccupancy >> 14) & 3) < 2);
		return isFree;
	}

	void FOctTreeChild::GetChildFromDelta(const FVector3f& Delta, float HalfChildSize, uint32& OutIndex, FVector3f& OutOrigin)
	{
		const uint32 SignX = static_cast<uint32>(FMath::FloatSelect(Delta.X, 1.f, 0.f));
		const uint32 SignY = static_cast<uint32>(FMath::FloatSelect(Delta.Y, 1.f, 0.f));
		const uint32 SignZ = static_cast<uint32>(FMath::FloatSelect(Delta.Z, 1.f, 0.f));
		
		OutIndex = SignX + SignY * 2 + SignZ * 4;
		
		const float DoubleSize = HalfChildSize * 2.0f;
		OutOrigin.X = DoubleSize * SignX - HalfChildSize;
		OutOrigin.Y = DoubleSize * SignY - HalfChildSize;
		OutOrigin.Z = DoubleSize * SignZ - HalfChildSize;
	}

	void FOctTreeChild::SetChildOccupancy(uint16& InChildOccupancy, uint32 Index, uint16 Mask)
	{
		const uint32 Shift = Index * 2; 
		const uint16 ChildMask = Mask << Shift;
		InChildOccupancy &= ~(3 << Shift);
		InChildOccupancy |= ChildMask;
	}

	bool FOctTreeChild::IsChildMatch(uint16 InChildOccupancy, uint32 Index, uint16 Mask)
	{
		InChildOccupancy = InChildOccupancy >> (Index * 2);
		InChildOccupancy &= 3;
		return InChildOccupancy == Mask;
	}
}
