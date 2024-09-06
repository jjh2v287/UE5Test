// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "UKWidget.generated.h"

/**
 * 
 */
UCLASS()
class UETEST_API UUKWidget : public UUserWidget
{
	GENERATED_BODY()
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void FindScreenEdgeLocationForWorldLocation(UObject* WorldContextObject, const FVector& InLocation, const float EdgePercent,  FVector2D& OutScreenPosition, float& OutRotationAngleDegrees, bool &bIsOnScreen);

protected:
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget))
	TObjectPtr<UImage> Marker;
};
