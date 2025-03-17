// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeAIComponent.h"
#include "UKStateTreeAIComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UUKStateTreeAIComponent : public UStateTreeAIComponent
{
	GENERATED_BODY()

public:
	//~ BEGIN IStateTreeSchemaProvider
	TSubclassOf<UStateTreeSchema> GetSchema() const override;
	//~ END
	
	virtual bool SetContextRequirements(FStateTreeExecutionContext& Context, bool bLogErrors = false) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FVector> VectorPram;
};
