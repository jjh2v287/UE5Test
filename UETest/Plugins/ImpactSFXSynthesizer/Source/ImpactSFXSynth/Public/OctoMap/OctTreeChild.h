// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace OctoMap
{
    enum class EOctChildMask: uint16
    {
    	Unknown = 0,
    	Occupied = 1,
    	Free = 2,
    	Inner = 3,
    	AllFree = 0xAAAA,
    	AllOccupied = 0x5555,
    };
	
	struct FOctBox
	{
		FVector Origin;
		FVector Extend;

		FOctBox(const FVector& InOrigin, const FVector& InExtend)
			: Origin(InOrigin), Extend(InExtend) {}
	};

    class IMPACTSFXSYNTH_API FOctTreeChild
	{
	public:
		static constexpr int32 NumChild = 8;
    	static TArray<FInt32Vector> ChildOriginMap;
    	
		static void GetChildFromDelta(const FVector3f& Delta, float HalfChildSize, uint32& OutIndex, FVector3f& OutOrigin);
		static void SetChildOccupancy(uint16& InChildOccupancy, uint32 Index, uint16 Mask);
		static bool IsChildMatch(uint16 InChildOccupancy, uint32 Index, uint16 Mask);
		
		FOctTreeChild();
    	
		virtual ~FOctTreeChild() = default;
		
		bool UpdateOccupancy(const FVector3f& DeltaVector, float HalfChildSize, uint16 Free, int32 CurrentDepth, int32 MaxDepth);
    	
    	void GetAllOccupiedLocation(TArray<FOctBox>& OutArray, const FVector& ChildOrigin, float ChildSize);

	private:
		FORCEINLINE uint16 GetFreeOrOccupiedMask(uint16 Free);
    	FORCEINLINE bool IsCanBePruned() const;
		bool IsAllFreeOrUnknown() const;

	private:
		uint16 ChildOccupancy;
		TArray<TSharedPtr<FOctTreeChild>> ChildNodes;
	};
}