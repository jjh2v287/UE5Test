// Copyright 2024 BitProtectStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/UserDefinedStruct.h"

#include "WDActorComponent.generated.h"

UCLASS(Blueprintable)
class WORLDDIRECTORACTOR_API UWDActorComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPrepareForOptimization);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRecoveryFromOptimization);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBehindCameraFOV);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInCameraFOV);

public:	
	// Sets default values for this component's properties
	UWDActorComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// Broadcasts notification
	void BroadcastOnPrepareForOptimization() const;
	// Broadcasts notification
	void BroadcastOnRecoveryFromOptimization() const;

	// Broadcasts notification
	void BroadcastBehindCameraFOV() const;
	// Broadcasts notification
	void BroadcastInCameraFOV() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Director Actor Parameters")
    void PrepareForOptimizationBP();
	UFUNCTION(BlueprintImplementableEvent, Category = "Director Actor Parameters")
    void RecoveryFromOptimization();

	UFUNCTION(BlueprintImplementableEvent, Category = "Director Actor Parameters")
    void BehindCameraFOV();
	UFUNCTION(BlueprintImplementableEvent, Category = "Director Actor Parameters")
    void InCameraFOV();

	void SetActorUniqName(FString setName);
	FString GetActorUniqName();

	// Delegate
	UPROPERTY(BlueprintAssignable)
	FPrepareForOptimization OnPrepareForOptimization;
	UPROPERTY(BlueprintAssignable)
	FRecoveryFromOptimization OnRecoveryFromOptimization;

	// Delegate
	UPROPERTY(BlueprintAssignable)
	FPrepareForOptimization EventBehindCameraFOV;
	UPROPERTY(BlueprintAssignable)
	FRecoveryFromOptimization EventInCameraFOV;

	// Set Enable Director Actor Component.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	bool bIsActivate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	bool bIsOptimizeAllActorComponentsTickInterval = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	bool bIsDisableTickIfBehindCameraFOV = true;

	protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void RegisterActorTimer();

	void OptimizationTimer();

	bool IsCameraSeeActor() const;
	

	private:

	TArray<float> defaultTickComponentsInterval;
	float hiddenTickComponentsInterval;

	float defaultTickActorInterval;
	float hiddenTickActorInterval;

	float distanceCamara = 1000.f;

	FName componentsTag = "WDA";

	UPROPERTY()
	class ADirectorActor *directorActorRef;

	FTimerHandle registerActor_Timer;

	bool bIsRegistered = false;

	FTimerHandle actorOptimization_Timer;

	UPROPERTY()
	AActor* ownerActor = nullptr;

	FString actorUniqName = "";	
};
