// Copyright Epic Games, Inc. All Rights Reserved.

#include "GridSpawner.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

// 생성자
AGridSpawner::AGridSpawner()
{
	// 매 프레임 Tick이 필요하지 않으므로 비활성화
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트 생성
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

// 게임 시작 시 호출
void AGridSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	// 게임 시작 시 자동 스폰 설정이 활성화되어 있으면 액터 스폰
	if (bSpawnOnBeginPlay)
	{
		SpawnActorsInGrid();
	}
}

// 에디터에서 프로퍼티 변경 시 호출
void AGridSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 에디터에서 변경 시 자동 리스폰 설정이 활성화되어 있으면 액터 스폰
	if (bAutoRespawnOnChange)
	{
		// 이미 스폰된 액터 제거
		ClearSpawnedActors();
		// 새로운 설정으로 액터 스폰
		SpawnActorsInGrid();
	}
}

// 격자 내 위치 계산 함수
FVector AGridSpawner::CalculateGridPosition(int32 IndexX, int32 IndexY, int32 IndexZ) const
{
	// 스포너 위치를 중심으로 격자 위치 계산
	// X, Y축은 중앙에서 시작하여 양쪽으로 퍼지도록 계산
	// Z축은 바닥부터 시작하도록 계산
	float OffsetX = (GridSizeX - 1) * 0.5f;
	float OffsetY = (GridSizeY - 1) * 0.5f;
	
	return FVector(
		(IndexX - OffsetX) * GridSpacing,
		(IndexY - OffsetY) * GridSpacing,
		IndexZ * GridSpacing
	);
}

// 격자 형태로 액터 스폰 함수
void AGridSpawner::SpawnActorsInGrid()
{
	// 월드 객체 확인
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("GridSpawner: World is invalid"));
		return;
	}

	// 스폰할 액터 클래스 확인
	if (!ActorToSpawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridSpawner: No actor blueprint specified to spawn"));
		return;
	}

	// 이미 스폰된 액터 제거
	ClearSpawnedActors();

	// 스폰된 액터 수 추적 변수
	int32 SpawnedCount = 0;

	// X, Y, Z 축으로 순회하며 액터 스폰
	for (int32 Z = 0; Z < GridSizeZ; Z++)
	{
		for (int32 Y = 0; Y < GridSizeY; Y++)
		{
			for (int32 X = 0; X < GridSizeX; X++)
			{
				// 최대 스폰 액터 수 체크
				if (SpawnedCount >= MaxActorsToSpawn)
				{
					UE_LOG(LogTemp, Warning, TEXT("GridSpawner: Reached maximum number of actors to spawn (%d)"), MaxActorsToSpawn);
					return;
				}

				// 스폰 위치 계산
				FVector SpawnLocation = GetActorLocation() + CalculateGridPosition(X, Y, Z);
				
				// 스폰 회전 설정
				FRotator SpawnRotation = GetActorRotation();
				
				// 회전 랜덤화 설정이 활성화되어 있으면 랜덤 회전 적용
				if (bRandomizeRotation)
				{
					SpawnRotation = FRotator(
						FMath::FRandRange(0.0f, 360.0f),
						FMath::FRandRange(0.0f, 360.0f),
						FMath::FRandRange(0.0f, 360.0f)
					);
				}

				FString ActorName = FString::Printf(TEXT("Actor_%d"), SpawnedCount + 1);
				
				// 액터 스폰 설정
				FActorSpawnParameters SpawnParams;
				SpawnParams.Name = FName(*ActorName);
				SpawnParams.Owner = this;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				// 액터 스폰
				AActor* SpawnedActor = World->SpawnActor<AActor>(ActorToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
				SpawnedActor->SetActorLabel(ActorName);
				
				// 스폰 성공 체크
				if (SpawnedActor)
				{
					// 크기 랜덤화 설정이 활성화되어 있으면 랜덤 크기 적용
					if (bRandomizeScale)
					{
						float RandomScale = FMath::FRandRange(MinScale, MaxScale);
						SpawnedActor->SetActorScale3D(FVector(RandomScale));
					}

					// 스폰된 액터 배열에 추가
					SpawnedActors.Add(SpawnedActor);
					SpawnedCount++;
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("GridSpawner: Successfully spawned %d actors"), SpawnedCount);
}

// 이미 스폰된 액터 제거 함수
void AGridSpawner::ClearSpawnedActors()
{
	// 이전에 스폰된 모든 액터 제거
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}

	// 배열 초기화
	SpawnedActors.Empty();
}