// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UKNPCUpdateInterface.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "UKNPCUpdateManager.generated.h"


USTRUCT()
struct FManagedActorData
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	AActor* OwnerActor = nullptr;

	IUKNPCUpdateInterface* InterfacePtr = nullptr;

	UPROPERTY(Transient)
	float AccumulatedTime = 0.f;
	
	UPROPERTY(Transient)
	float Distance = 0.0f;

	FManagedActorData() {}
	FManagedActorData(AActor* InActor)
	{
		OwnerActor = InActor;
		InterfacePtr = Cast<IUKNPCUpdateInterface>(InActor);
	}
};

UCLASS()
class UKNPC_API UUKNPCUpdateManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// USubsystem implementation End

	// FTickableGameObject implementation Begin
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }
	// FTickableGameObject implementation End
	
	void Register(AActor* InActor);
	void Unregister(AActor* InActor);
	
private:
	
	void RebalanceGroups();
	UFUNCTION()
	void OnActorDestroyed(AActor* DestroyedActor);
	
	// 전체 데이터 관리
	// [수정] 구조체 값을 직접 담지 않고, 스마트 포인터를 담습니다.
	// 이렇게 하면 배열이 리사이징 되어도 실제 데이터의 메모리 주소는 변하지 않습니다.
	TArray<TSharedPtr<FManagedActorData>> MasterList;

	// 그룹별 포인터 리스트 (정렬 및 참조용)
	TArray<FManagedActorData*> HighPriorityGroup; // 근거리 (VIP)
	TArray<FManagedActorData*> LowPriorityGroup;  // 원거리 (Budget 적용)

	// 가장 가까운 10마리는 VIP
	const int32 HighPriorityCount = 10;
	// 전체 NPC 처리에 쓸 최대 시간 (2ms)
	const double FrameBudgetMs = 2.0;

	// 재분배 타이머
	float RebalanceTimer = 0.f;
	// n초마다 거리 재계산
	const float RebalanceInterval = 0.5f;

	// Low Group용 이어달리기 커서
	int32 LowGroupCursor = 0;
};
