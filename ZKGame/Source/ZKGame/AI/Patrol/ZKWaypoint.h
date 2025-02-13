// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZKWaypoint.generated.h"

UCLASS()
class ZKGAME_API AZKWaypoint : public AActor
{
	GENERATED_BODY()

public:
	AZKWaypoint(const FObjectInitializer& ObjectInitializer);

	// 이전 경로 포인트들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint System")
	TArray<AZKWaypoint*> PreviousPathPoints;

	// 다음 경로 포인트들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint System")
	TArray<AZKWaypoint*> NextPathPoints;

	// 디버그 표시 활성화
	UPROPERTY(EditAnywhere, Category = "Waypoint System|Debug")
	bool bShowDebugLines{true};

	// 디버그 라인 색상 설정
	UPROPERTY(EditAnywhere, Category = "Waypoint System|Debug")
	FColor PreviousPathColor;

	UPROPERTY(EditAnywhere, Category = "Waypoint System|Debug")
	FColor NextPathColor;

	// 다음 웨이포인트 랜덤 선택
	UFUNCTION(BlueprintCallable, Category = "Waypoint System")
	AZKWaypoint* GetRandomNextWaypoint() const;

	// 이전 웨이포인트 랜덤 선택
	UFUNCTION(BlueprintCallable, Category = "Waypoint System")
	AZKWaypoint* GetRandomPreviousWaypoint() const;

	// 경로 유효성 검증
	UFUNCTION(BlueprintCallable, Category = "Waypoint System")
	bool ValidateConnections();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	};

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void DrawDebugLines();
#endif
};