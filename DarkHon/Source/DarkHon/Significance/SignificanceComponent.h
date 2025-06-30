// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SignificanceManager.h"
#include "Components/ActorComponent.h"
#include "SignificanceComponent.generated.h"

class AActor;
class UCharacterMovementComponent;
class UBehaviorTreeComponent;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct FSignificanceThreshold
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float Significance = 0.0f;
	UPROPERTY(EditDefaultsOnly)
	float MaxDistance = 0.0f;
	UPROPERTY(EditDefaultsOnly)
	float TickInterval = 0.0f;
	UPROPERTY(EditDefaultsOnly)
	float AnimationFrameSkipCount = 0;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DARKHON_API USignificanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USignificanceComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	float SignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& Viewpoint);
	float GetSignificanceByDistance(float Distance);
	void PostSignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal);
	void SetTickEnabled(bool bEnabled);
	void DrawDebugText(float Significance) const;

protected:
	float LastLevel = 0.0f;
	int32 SignificanceCount = 0;
	int32 SignificanceLastIndex = 0;

	UPROPERTY(EditDefaultsOnly)
	bool bActivated = true;
	UPROPERTY(EditDefaultsOnly)
	bool bDistanceURO = true;
	UPROPERTY(EditDefaultsOnly)
	int32 MaxEvalRateForInterpolation = 10;
	UPROPERTY(EditDefaultsOnly)
	int32 BaseNonRenderedUpdateRate = 6;
	
	UPROPERTY(Transient)
	AActor* OwnerActor = nullptr;
	UPROPERTY(Transient)
	UCharacterMovementComponent* MovementComponent = nullptr;
	UPROPERTY(Transient)
	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
	UPROPERTY(Transient)
	UBehaviorTreeComponent* BehaviorTreeComponent = nullptr;
	
	UPROPERTY(EditDefaultsOnly)
	TArray<FSignificanceThreshold> SignificanceThresholds
	{
			{ 0.0f, 3000.0f, 0.0f, 0  },
			{ 1.0f, 5000.0f, 0.0333f, 2  },	
			{ 2.0f, 7000.0f, 0.1166f, 7  },
			{ 3.0f, 9000.0f, 0.3333f, 20 }
	};
};
