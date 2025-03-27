#pragma once

#include "CoreMinimal.h"
#include "NavigationInvokerComponent.h"
#include "GameFramework/Actor.h"
#include "UKWayPoint.generated.h" // .h 파일 이름 확인

// 전방 선언 (필요 시)
class UUKHPAManager; // .cpp 파일에서 include 하므로 여기서는 전방선언 가능

UCLASS()
class ZKGAME_API AUKWayPoint : public AActor
{
	GENERATED_BODY()
public:
	AUKWayPoint(const FObjectInitializer& ObjectInitializer);
    
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual void Tick(float DeltaTime) override;

	/**
	* 이 웨이포인트가 속한 클러스터의 ID입니다.
	* 레벨 디자인 타임에 설정하거나, 자동 클러스터링 알고리즘으로 할당할 수 있습니다.
	* -1 또는 INVALID_CLUSTER_ID는 할당되지 않음을 의미할 수 있습니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint", meta = (DisplayName = "HPA Cluster ID")) // DisplayName 추가
	int32 ClusterID = -1;
	
	// 연결된 웨이포인트들의 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Waypoint")
	TArray<AUKWayPoint*> PathPoints;

	virtual bool ShouldTickIfViewportsOnly() const override { return true; };

protected:
	UPROPERTY(VisibleDefaultsOnly)
	UNavigationInvokerComponent* InvokerComponent{nullptr};

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};