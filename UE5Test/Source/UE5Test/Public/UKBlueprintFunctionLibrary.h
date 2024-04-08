// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Math/Vector2D.h"
#include "InputCoreTypes.h"
#include "UKBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class UE5TEST_API UUKBlueprintFunctionLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "UKGame|Actor")
	static FVector GetMiniMapUIRotationLocation(FVector Location, float Angle, FVector NewLocation, float NewAngle);
};
