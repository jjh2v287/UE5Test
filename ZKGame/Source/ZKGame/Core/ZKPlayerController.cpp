// Fill out your copyright notice in the Description page of Project Settings.


#include "ZKPlayerController.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AZKPlayerController::AZKPlayerController(const FObjectInitializer& ObjectInitializer)
   :Super(ObjectInitializer)
{
   bShowMouseCursor = false;
}

void AZKPlayerController::BeginPlay()
{
   Super::BeginPlay();
}