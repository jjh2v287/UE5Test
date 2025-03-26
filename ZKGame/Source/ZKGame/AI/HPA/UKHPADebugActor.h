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
    
    virtual void Tick(float DeltaTime) override;
    
    // HPA* 초기화 및 경로 탐색 테스트
    UFUNCTION(BlueprintCallable, Category="HPA Debug")
    void BuildHPA(float ClusterSize = 1000.0f);
    
    // 경로 탐색 테스트
    UFUNCTION(BlueprintCallable, Category="HPA Debug")
    void TestHPAPath();
    
    // 시각화 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HPA Debug|Visualization")
    bool bDrawClusters = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HPA Debug|Visualization")
    bool bDrawGateways = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HPA Debug|Visualization")
    bool bDrawAbstractGraph = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HPA Debug|Visualization")
    bool bDrawTestPath = true;
    
    // 테스트 경로 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HPA Debug|Test Path")
    AActor* StartActor;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HPA Debug|Test Path")
    AActor* EndActor;
    
    // 디버그 드로잉 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HPA Debug|Debug")
    float DebugSphereRadius = 30.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HPA Debug|Debug")
    float DebugArrowSize = 30.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HPA Debug|Debug")
    FColor TestPathColor = FColor::Yellow;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HPA Debug|Debug")
    float DebugThickness = 2.0f;
    
    virtual bool ShouldTickIfViewportsOnly() const override
    {
        return true;
    }
    
protected:
    virtual void BeginPlay() override;
    
private:
    // 현재 테스트 경로
    TArray<AUKWaypoint*> CurrentPath;
    
    // 경로 시각화
    void DrawTestPath();
};