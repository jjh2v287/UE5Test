#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UKHPADebugActor.generated.h" // 헤더 이름 확인

// 전방 선언
class UUKHPAManager;
class AUKWayPoint;

/**
 * HPA* 디버깅용 액터
 * 클러스터, 게이트웨이, 경로 등을 시각화
 */
UCLASS()
class ZKGAME_API AUKHPADebugActor : public AActor // 클래스 이름 확인
{
    GENERATED_BODY()

public:
    AUKHPADebugActor(); // 생성자 이름 확인

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
    FColor DebugPathColor = FColor::Cyan; // 이름 변경: PathColor

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
    float DebugPathThickness = 3.0f; // 이름 변경: PathThickness

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug", meta = (DisplayName = "Draw HPA Structure"))
    bool bDrawHPAStructure = true; // HPA 구조 디버그 드로잉 활성화

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug", meta = (DisplayName = "Debug Draw Duration (0=Tick)"))
    float DebugDrawDuration = 0.f; // 매 틱 업데이트 시 0, 특정 시간 동안 보려면 0보다 큰 값

    virtual bool ShouldTickIfViewportsOnly() const override { return true; }

private:
    void UpdatePathVisualization();

    // 경로 시각화를 위해 마지막 경로 저장 (선택적)
    UPROPERTY(Transient) // 저장 안 함
    TArray<TObjectPtr<AUKWayPoint>> LastFoundPath; // TObjectPtr 사용 권장
};