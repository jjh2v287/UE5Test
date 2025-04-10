// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridSpawner.generated.h"

UCLASS()
class ZKGAME_API AGridSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// 생성자
	AGridSpawner();

protected:
	// 게임 시작 시 호출되는 함수
	virtual void BeginPlay() override;

	// 에디터에서 변수 값이 변경됐을 때 호출
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:	
	// 격자 형태로 액터 스폰 함수
	UFUNCTION(BlueprintCallable, Category = "Grid Spawner")
	void SpawnActorsInGrid();

	// 이미 스폰된 액터 제거
	UFUNCTION(BlueprintCallable, Category = "Grid Spawner")
	void ClearSpawnedActors();

private:
	// 격자 내 위치 계산 함수
	FVector CalculateGridPosition(int32 IndexX, int32 IndexY, int32 IndexZ) const;

private:
	// 스폰할 액터 클래스 (블루프린트에서 설정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor> ActorToSpawn;

	// X축 방향 격자 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 GridSizeX = 20;

	// Y축 방향 격자 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 GridSizeY = 20;

	// Z축 방향 격자 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 GridSizeZ = 1;

	// 격자 간격
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float GridSpacing = 100.0f;

	// 최대 스폰 액터 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxActorsToSpawn = 2000;

	// 회전 랜덤화 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true"))
	bool bRandomizeRotation = false;

	// 크기 랜덤화 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true"))
	bool bRandomizeScale = false;

	// 최소 스케일 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true", EditCondition = "bRandomizeScale", ClampMin = "0.1", ClampMax = "10.0"))
	float MinScale = 0.8f;

	// 최대 스케일 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true", EditCondition = "bRandomizeScale", ClampMin = "0.1", ClampMax = "10.0"))
	float MaxScale = 1.2f;

	// 게임 시작 시 자동 스폰 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true"))
	bool bSpawnOnBeginPlay = true;

	// 에디터에서 프로퍼티 변경 시 자동 스폰 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Spawner", meta = (AllowPrivateAccess = "true"))
	bool bAutoRespawnOnChange = false;

	// 생성된 액터 배열
	UPROPERTY()
	TArray<AActor*> SpawnedActors;
};