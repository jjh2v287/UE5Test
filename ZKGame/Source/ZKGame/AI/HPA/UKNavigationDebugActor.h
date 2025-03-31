// Copyright Kong Studios, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UKNavigationDebugActor.generated.h"

// 전방 선언
class UUKNavigationManager;
class AUKWayPoint;

/**
 * HPA* 디버깅용 액터
 * 클러스터, 게이트웨이, 경로 등을 시각화
 */
UCLASS()
class ZKGAME_API AUKNavigationDebugActor : public AActor
{
    GENERATED_BODY()

public:
    AUKNavigationDebugActor();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing")
    AActor* EndActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    float DebugSphereRadius = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    float DebugArrowSize = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    FColor DebugPathColor = FColor::Cyan;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    float DebugPathThickness = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug", meta = (DisplayName = "Draw HPA Structure"))
    bool bDrawHPAStructure = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug", meta = (DisplayName = "Debug Draw Duration (0=Tick)"))
    float DebugDrawDuration = 0.f;

    virtual bool ShouldTickIfViewportsOnly() const override { return true; }

private:
    void UpdatePathVisualization();

    // 경로 시각화를 위해 마지막 경로 저장 (선택적)
    UPROPERTY(Transient)
    TArray<TObjectPtr<AUKWayPoint>> LastFoundPath;
};