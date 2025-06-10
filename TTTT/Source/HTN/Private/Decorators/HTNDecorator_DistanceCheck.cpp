// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Decorators/HTNDecorator_DistanceCheck.h"
#include "HTNObjectVersion.h"
#include "WorldStateProxy.h"
#include "Utility/HTNHelpers.h"

#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

UHTNDecorator_DistanceCheck::UHTNDecorator_DistanceCheck(const FObjectInitializer& Initializer) : Super(Initializer),
	DistanceRange(FFloatRangeBound::Open(), FFloatRangeBound::Inclusive(1000)),
	CheckMode(EHTNDecoratorDistanceCheckMode::Distance3D),
	CapsuleHalfHeight(90.0f),
	CapsuleRadius(0.0f),
	bAllMustPass(true)
{
	LocationSourceA.InitializeBlackboardKeySelector(this, GET_MEMBER_NAME_CHECKED(UHTNDecorator_DistanceCheck, LocationSourceA));
	LocationSourceB.InitializeBlackboardKeySelector(this, GET_MEMBER_NAME_CHECKED(UHTNDecorator_DistanceCheck, LocationSourceB));
}

void UHTNDecorator_DistanceCheck::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FHTNObjectVersion::GUID);
	const int32 HTNObjectVersion = Ar.CustomVer(FHTNObjectVersion::GUID);

	if (HTNObjectVersion < FHTNObjectVersion::MakeDistanceCheckMoreFlexible)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		LocationSourceA.BlackboardKey = A_DEPRECATED;
		LocationSourceB.BlackboardKey = B_DEPRECATED;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	if (HTNObjectVersion < FHTNObjectVersion::MakeDistanceCheckUseFloatRange)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		DistanceRange = FFloatRange(
			FFloatRangeBound::Inclusive(MinDistance_DEPRECATED), 
			FFloatRangeBound::Inclusive(MaxDistance_DEPRECATED));
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
}

void UHTNDecorator_DistanceCheck::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);

	LocationSourceA.InitializeFromAsset(Asset);
	LocationSourceB.InitializeFromAsset(Asset);
}

FString UHTNDecorator_DistanceCheck::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	switch (CheckMode)
	{
	case EHTNDecoratorDistanceCheckMode::Distance3D:
		return TEXT("Distance Check");
	case EHTNDecoratorDistanceCheckMode::Distance2D:
		return TEXT("Distance Check 2D");
	case EHTNDecoratorDistanceCheckMode::DistanceSignedZ:
		return TEXT("Distance Check Signed Z");
	case EHTNDecoratorDistanceCheckMode::DistanceAbsoluteZ:
		return TEXT("Distance Check Absolute Z");
	case EHTNDecoratorDistanceCheckMode::Capsule:
		return TEXT("Distance Check Capsule");
	default:
		checkNoEntry();
		return TEXT("Distance Check Invalid");
	}
}

FString UHTNDecorator_DistanceCheck::GetStaticDescription() const
{
	TStringBuilder<2048> SB;

	SB << Super::GetStaticDescription();
	SB << (IsInversed() ? TEXT("\nDistance not in range: ") : TEXT("\nDistance in range: "));
	SB << FHTNHelpers::DescribeRange(DistanceRange) << TEXT(" cm");

	SB << TEXT("\nLocationA: ") << LocationSourceA.ToString();
	SB << TEXT("\nLocationB: ") << LocationSourceB.ToString();

	if (CheckMode == EHTNDecoratorDistanceCheckMode::Capsule)
	{
		SB << TEXT("\nCapsuleHalfHeight: ") << FString::SanitizeFloat(CapsuleHalfHeight) << TEXT("cm");
		SB << TEXT("\nCapsuleRadius: ") << FString::SanitizeFloat(CapsuleRadius) << TEXT("cm");
	}

	return SB.ToString();
}

bool UHTNDecorator_DistanceCheck::CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const
{
	UWorldStateProxy* const WorldStateProxy = GetWorldStateProxy(OwnerComp, CheckType);
	if (!ensure(WorldStateProxy))
	{
		return false;
	}

	LocationsABuffer.Reset();
	LocationSourceA.GetLocations(LocationsABuffer, OwnerComp.GetOwner(), *WorldStateProxy);
	if (!LocationsABuffer.Num())
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Warning, TEXT("%s: %s has no valid locations!"),
			*GetShortDescription(), *LocationSourceA.ToString());
		return false;
	}

	LocationsBBuffer.Reset();
	LocationSourceB.GetLocations(LocationsBBuffer, OwnerComp.GetOwner(), *WorldStateProxy);
	if (!LocationsBBuffer.Num())
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Warning, TEXT("%s: %s has no valid locations!"),
			*GetShortDescription(), *LocationSourceB.ToString());
		return false;
	}

	const bool bSuccess = [&]() -> bool
	{
		for (const FVector& A : LocationsABuffer)
		{
			for (const FVector& B : LocationsBBuffer)
			{
				const float Distance = CalculateDistance(A, B);
				const bool bPass = DistanceRange.Contains(Distance);
				UE_VLOG_SEGMENT(&OwnerComp, LogHTN, VeryVerbose, A, B, (IsInversed() ^ bPass) ? FColor::Green : FColor::Black,
					TEXT("%.2fcm: %s %s"), Distance, *GetShortDescription(), *FHTNHelpers::DescribeRange(DistanceRange));

				if (bAllMustPass && !bPass)
				{
					return false;
				}
				else if (!bAllMustPass && bPass)
				{
					return true;
				}
			}
		}

		return bAllMustPass;
	}();

	return bSuccess;
}

float UHTNDecorator_DistanceCheck::CalculateDistance(const FVector& A, const FVector& B) const
{
	switch (CheckMode)
	{
	case EHTNDecoratorDistanceCheckMode::Distance3D:
		return FVector::Dist(A, B);
	case EHTNDecoratorDistanceCheckMode::Distance2D:
		return FVector::Dist2D(A, B);
	case EHTNDecoratorDistanceCheckMode::DistanceSignedZ:
		return B.Z - A.Z;
	case EHTNDecoratorDistanceCheckMode::DistanceAbsoluteZ:
		return FMath::Abs(B.Z - A.Z);
	case EHTNDecoratorDistanceCheckMode::Capsule:
	{
		const float DistanceToSegmentInsideCapsule = FMath::PointDistToSegment(B, 
			A - FVector(0.0f, 0.0f, CapsuleHalfHeight), 
			A + FVector(0.0f, 0.0f, CapsuleHalfHeight));
		return FMath::Max(0.0f, DistanceToSegmentInsideCapsule - CapsuleRadius);
	}
	default:
		checkNoEntry();
		return 0.0f;
	}
}
