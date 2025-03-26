#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UKHPAManager.h"
#include "UKHPADebugActor.generated.h"

/**
 * HPA* 디버깅용 액터
 * 클러스터, 게이트웨이, 경로 등을 시각화
 */
UCLASS()
class ZKGAME_API AUKHPADebugActor : public AActor
{
    GENERATED_BODY()
    
public:    
    AUKHPADebugActor();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    
public:
    // 시작점과 끝점 액터 참조
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing")
    AActor* EndActor;
    
    // 디버그 드로잉 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    float DebugSphereRadius = 30.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    float DebugArrowSize = 30.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    FColor DebugColor = FColor::Red;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    float DebugThickness = 1.0f;
    
    virtual bool ShouldTickIfViewportsOnly() const override
    {
        return true;
    }

private:
    void UpdatePathVisualization();
};