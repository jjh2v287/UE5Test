// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UKPatrolPathSpline.generated.h"

UCLASS()
class ZKGAME_API AUKPatrolPathSpline : public AActor
{
	GENERATED_BODY()

public:
	AUKPatrolPathSpline(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spline")
	TObjectPtr<USplineComponent> SplineComponent;
};
