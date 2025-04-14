// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UKECSEntity.h"
#include "UKECSSystemBase.h"
#include "Components/ActorComponent.h"
#include "StructUtils/InstancedStruct.h"
#include "UKECSAgentComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ZKGAME_API UUKECSAgentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UUKECSAgentComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite ,Category = "ECS")
	bool RunECS = true;

	UFUNCTION(BlueprintCallable)
	void InitData();
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly ,Category = "ECS")
	TArray<TSubclassOf<UUKECSSystemBase>> ECSSystemBaseClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ECS", meta = (BaseStruct = "/Script/ZKGame.UKECSComponentBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> ECSComponentInstances;

	FECSEntityHandle Entity;
};
