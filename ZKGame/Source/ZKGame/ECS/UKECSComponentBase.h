// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UKECSComponentBase.generated.h"

/** 모든 컴포넌트의 기본 구조체 */
USTRUCT(BlueprintType)
struct ZKGAME_API FUKECSComponentBase
{
	GENERATED_BODY()
};

// --- 예시 컴포넌트 타입들 ---
USTRUCT(BlueprintType)
struct ZKGAME_API FUKECSPositionComponent : public FUKECSComponentBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "ECS")
	FVector Position = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct ZKGAME_API FUKECSVelocityComponent : public FUKECSComponentBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "ECS")
	FVector Velocity = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct ZKGAME_API FUKECSMovementSpeedComponent : public FUKECSComponentBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "ECS")
	float Speed = 100.0f;
};