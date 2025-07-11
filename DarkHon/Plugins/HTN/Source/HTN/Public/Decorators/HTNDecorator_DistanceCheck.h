// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNDecorator.h"
#include "Utility/HTNLocationSource.h"
#include "HTNDecorator_DistanceCheck.generated.h"

UENUM()
enum class EHTNDecoratorDistanceCheckMode
{
	// 3D distance between A and B
	Distance3D,
	// 2D distance between A and B
	Distance2D,
	// Signed vertical distance: (B.Z - A.Z). Will be negative if B is below A.
	DistanceSignedZ UMETA(DisplayName = "Distance Z (Signed)"),
	// Absolute vertical distance
	DistanceAbsoluteZ UMETA(DisplayName = "Distance Z (Absolute)"),
	// If there was a capsule centered at A with the specified half-height and radius, calculates the distance from B to that capsule.
	// Swapping A and B has no effect.
	Capsule
};

// Checks if the distance between two worldstate keys is smaller than a specified distance.
UCLASS()
class HTN_API UHTNDecorator_DistanceCheck : public UHTNDecorator
{
	GENERATED_BODY()

public:
	UHTNDecorator_DistanceCheck(const FObjectInitializer& Initializer);
	virtual void Serialize(FArchive& Ar) override;
	virtual void InitializeFromAsset(UHTN& Asset) override;
	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;

	UPROPERTY(EditAnywhere, Category = Node)
	FHTNLocationSource LocationSourceA;

	UPROPERTY(EditAnywhere, Category = Node)
	FHTNLocationSource LocationSourceB;

	// The distance between LocationSourceA and LocationSourceB has to be in this range
	UPROPERTY(EditAnywhere, Category = Node, Meta = (ForceUnits = cm))
	FFloatRange DistanceRange;

	UPROPERTY(EditAnywhere, Category = Node)
	EHTNDecoratorDistanceCheckMode CheckMode;

	UPROPERTY(EditAnywhere, Category = Node, Meta = (ForceUnits = cm, ClampMin = 0, EditCondition = "CheckMode == EHTNDecoratorDistanceCheckMode::Capsule", EditConditionHides))
	float CapsuleHalfHeight;

	UPROPERTY(EditAnywhere, Category = Node, Meta = (ForceUnits = cm, ClampMin = 0, EditCondition = "CheckMode == EHTNDecoratorDistanceCheckMode::Capsule", EditConditionHides))
	float CapsuleRadius;

	// If one or both of the location sources produces have multiple values, must all combinations pass for the raw condition to be true. 
	// If false, just one will be enough.
	UPROPERTY(EditAnywhere, Category = "Node")
	uint8 bAllMustPass : 1;

	UPROPERTY()
	float MinDistance_DEPRECATED;

	UPROPERTY()
	float MaxDistance_DEPRECATED;

	UPROPERTY()
	FBlackboardKeySelector A_DEPRECATED;

	UPROPERTY()
	FBlackboardKeySelector B_DEPRECATED;

protected:
	virtual bool CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const override;

private:
	float CalculateDistance(const FVector& A, const FVector& B) const;

	// Temporary buffers to avoid extra allocations.
	mutable TArray<FVector> LocationsABuffer;
	mutable TArray<FVector> LocationsBBuffer;
};
