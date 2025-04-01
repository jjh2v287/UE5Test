// Copyright Kong Studios, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UKNavigationDebugActor.generated.h"

class UUKNavigationManager;
class AUKWayPoint;

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
    float DebugSphereRadius = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    float DebugArrowSize = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    FColor DebugPathColor = FColor::Cyan;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    float DebugPathThickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug", meta = (DisplayName = "Draw HPA Structure"))
    bool bDrawHPAStructure = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug", meta = (DisplayName = "Debug Draw Duration (0=Tick)"))
    float DebugDrawDuration = 0.0f;

    virtual bool ShouldTickIfViewportsOnly() const override { return true; }

private:
    void UpdatePathVisualization();

    UPROPERTY(Transient)
    TArray<TObjectPtr<AUKWayPoint>> LastFoundPath;
};