// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BlackboardWorldstate.h"
#include "WorldstateSetValueContainer.generated.h"

USTRUCT()
struct HTN_API FWorldstateSetValueContainer
{
	GENERATED_BODY()

	// Not using a TVariant for ease of editing and preserving values that aren't in use.
	
	UPROPERTY(Category = Blackboard, EditAnywhere, meta = (DisplayName = "Value"))
	int32 IntValue = 0;

	UPROPERTY(Category = Blackboard, EditAnywhere, meta = (DisplayName = "Value"))
	float FloatValue = 0.f;

	UPROPERTY(Category = Blackbaord, EditAnywhere, meta = (DisplayName = "Value"))
	FVector VectorValue = FVector::ZeroVector;

	UPROPERTY(Category = Blackbaord, EditAnywhere, meta = (DisplayName = "Value"))
	FRotator RotatorValue = FRotator::ZeroRotator;

	UPROPERTY(Category = Blackboard, EditAnywhere, meta = (DisplayName = "Value"))
	FString StringValue = TEXT("");
	
	UPROPERTY(Category = Blackboard, EditAnywhere, meta = (DisplayName = "Value"))
	FName NameValue = NAME_None;

	UPROPERTY(Category = Blackboard, EditAnywhere, meta = (DisplayName = "Value"))
	TObjectPtr<UObject> ObjectValue = nullptr;
	
	bool SetValue(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
	FString GetValueDescription(const UBlackboardData* BlackboardAsset, FBlackboard::FKey KeyID) const;
	FString GetValueDescription(const UBlackboardKeyType* ValueType) const;

	bool SetValueInt(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
	bool SetValueBool(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
	bool SetValueEnum(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
	bool SetValueNativeEnum(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
	bool SetValueFloat(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
	bool SetValueString(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
	bool SetValueName(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
	bool SetValueVector(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
	bool SetValueRotator(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
	bool SetValueClass(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
	bool SetValueObject(FBlackboardWorldState& WorldState, const FBlackboardKeySelector& KeySelector) const;
};
