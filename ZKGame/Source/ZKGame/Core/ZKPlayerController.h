// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ZKPlayerController.generated.h"

struct FInputActionValue;
/**
 * 
 */
UCLASS(Blueprintable)
class ZKGAME_API AZKPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	explicit AZKPlayerController(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
};
