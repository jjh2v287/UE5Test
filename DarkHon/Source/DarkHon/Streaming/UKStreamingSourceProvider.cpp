// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKStreamingSourceProvider.h"

#include "Components/WorldPartitionStreamingSourceComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKStreamingSourceProvider)

AUKStreamingSourceProvider::AUKStreamingSourceProvider(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WorldPartitionStreamingSource = CreateDefaultSubobject<UWorldPartitionStreamingSourceComponent>(TEXT("WorldPartitionStreamingSource"));
}

void AUKStreamingSourceProvider::BeginPlay()
{
	Super::BeginPlay();
}

void AUKStreamingSourceProvider::BeginDestroy()
{	
	Super::BeginDestroy();
}

void AUKStreamingSourceProvider::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AUKStreamingSourceProvider::ShapeGenerator()
{
	if (!WorldPartitionStreamingSource)
	{
		return;
	}

	WorldPartitionStreamingSource->Shapes.Empty();

	// 크기
    float ActorWidth = BoxExtent.X * 2.0f;
    float ActorHeight = BoxExtent.Y * 2.0f;
    
    // 필요한 분할 수 계산
    int32 DivisionsX = FMath::CeilToInt(ActorWidth / (MaxShapeRadius * 2.0f));
    int32 DivisionsY = FMath::CeilToInt(ActorHeight / (MaxShapeRadius * 2.0f));

    // 최소 1개씩은 생성하도록
    DivisionsX = FMath::Max(1, DivisionsX);
    DivisionsY = FMath::Max(1, DivisionsY);

    // 실제 각 Shape의 크기 계산
    float ShapeSpacingX = ActorWidth / DivisionsX;
    float ShapeSpacingY = ActorHeight / DivisionsY;
    float OverlapX = ShapeSpacingX;
    float OverlapY = ShapeSpacingY;

    // 중심 위치 가져오기
    FVector BoxCenter = FVector::ZeroVector;
    float StartX = BoxCenter.X - (ActorWidth * 0.5f) + (ShapeSpacingX * 0.5f);
    float StartY = BoxCenter.Y - (ActorHeight * 0.5f) + (ShapeSpacingY * 0.5f);

    // Shape 생성
    for (int32 Y = 0; Y < DivisionsY; Y++)
    {
        for (int32 X = 0; X < DivisionsX; X++)
        {
            FStreamingSourceShape Shape;
            Shape.Location = FVector(
                StartX + (X * ShapeSpacingX),
                StartY + (Y * ShapeSpacingY),
                BoxCenter.Z
            );

            // 반지름 계산
        	Shape.bUseGridLoadingRange = false;
            float RadiusX = (ShapeSpacingX + OverlapX) * 0.5f;
            float RadiusY = (ShapeSpacingY + OverlapY) * 0.5f;
            Shape.Radius = FMath::Min(FMath::Max(RadiusX, RadiusY), MaxShapeRadius);

            WorldPartitionStreamingSource->Shapes.Add(Shape);
        }
    }
}
#endif