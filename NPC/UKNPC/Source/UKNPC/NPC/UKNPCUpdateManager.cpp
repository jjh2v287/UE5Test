// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKNPCUpdateManager.h"
#include "Kismet/GameplayStatics.h"
#include "Algo/Sort.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKNPCUpdateManager)

void UUKNPCUpdateManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UUKNPCUpdateManager::Deinitialize()
{
	MasterList.Empty();
	Super::Deinitialize();
}

TStatId UUKNPCUpdateManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UUKNPCUpdateManager, STATGROUP_Tickables);
}

void UUKNPCUpdateManager::Register(AActor* InActor)
{
	if (!IsValid(InActor) || !InActor->Implements<UUKNPCUpdateInterface>())
	{
		return;
	}

	// 중복 방지
	for (const auto& Data : MasterList)
	{
		if (Data->OwnerActor == InActor)
		{
			return;
		}
	}

	InActor->SetActorTickEnabled(false);
	InActor->OnDestroyed.AddDynamic(this, &UUKNPCUpdateManager::OnActorDestroyed);

	TSharedPtr<FManagedActorData> NewData = MakeShared<FManagedActorData>(InActor);
	// 랜덤 오프셋으로 초기 부하 분산
	NewData->AccumulatedTime = -FMath::FRandRange(0.f, 0.05f); 
	
	MasterList.Add(NewData);
}

void UUKNPCUpdateManager::Unregister(AActor* InActor)
{
	// MasterList에서 제거
	// Swap 제거 사용 시 인덱스 관리가 복잡하므로 Rebalance 주기까지 기다리거나 선형 검색으로 제거
	
	const int32 Index = MasterList.IndexOfByPredicate([InActor](const TSharedPtr<FManagedActorData>& DataPtr){
		return DataPtr->OwnerActor == InActor;
	});

	if (Index != INDEX_NONE)
	{
		MasterList.RemoveAtSwap(Index);
		// 안전을 위해 즉시 갱신 RebalanceGroups() 호출
		RebalanceGroups(); 
	}
}

void UUKNPCUpdateManager::OnActorDestroyed(AActor* DestroyedActor)
{
	Unregister(DestroyedActor);
}

void UUKNPCUpdateManager::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(NPC_Budget_System);

	// 1. 모든 액터에게 시간 누적
	for (auto& Data : MasterList)
	{
		Data->AccumulatedTime += DeltaTime;
	}

	// 2. 주기적 거리 계산 및 그룹 재배치 (n초마다)
	RebalanceTimer += DeltaTime;
	if (RebalanceTimer >= RebalanceInterval)
	{
		RebalanceGroups();
		RebalanceTimer = 0.f;
	}

	// =========================================================
	// 예산 관리 시작
	// =========================================================
	const double StartTime = FPlatformTime::Seconds();
	const double BudgetSeconds = FrameBudgetMs / 1000.0;

	// ---------------------------------------------------------
	// A. High Priority Group (VIP) - 예산 무시하고 무조건 실행
	// ---------------------------------------------------------
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(NPC_HighPriority);
		for (FManagedActorData* DataPtr : HighPriorityGroup)
		{
			if (DataPtr && DataPtr->InterfacePtr && DataPtr->AccumulatedTime > 0.f)
			{
				DataPtr->InterfacePtr->ManualTick(DataPtr->AccumulatedTime);
				DataPtr->AccumulatedTime = 0.f;
			}
		}
	}

	// ---------------------------------------------------------
	// B. Low Priority Group (Rest) - 남은 예산으로 실행
	// ---------------------------------------------------------
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(NPC_LowPriority_Budgeted);
		
		int32 ProcessedCount = 0;
		const int32 TotalLow = LowPriorityGroup.Num();

		// Low 그룹이 비어있지 않다면
		if (TotalLow > 0)
		{
			// 한 바퀴 돌 때까지만 시도 (무한 루프 방지)
			while (ProcessedCount < TotalLow)
			{
				// 현재 시간 체크 (Budget Check)
				double CurrentTime = FPlatformTime::Seconds();
				if ((CurrentTime - StartTime) > BudgetSeconds)
				{
					// [예산 초과] 즉시 중단. 
					// Cursor 위치는 유지되므로 다음 프레임에 이어서 실행됨.
					break; 
				}

				// 커서 순환 (Wrap Around)
				LowGroupCursor = (LowGroupCursor + 1) % TotalLow;
				
				FManagedActorData* DataPtr = LowPriorityGroup[LowGroupCursor];
				
				if (DataPtr && DataPtr->InterfacePtr)
				{
					// [중요] Low 그룹은 자주 실행 안 되므로 AccumulatedTime이 큼 (예: 0.1초)
					float TimeToSimulate = DataPtr->AccumulatedTime;
					if (TimeToSimulate > 0.f)
					{
						DataPtr->InterfacePtr->ManualTick(TimeToSimulate);
						DataPtr->AccumulatedTime = 0.f;
					}
				}

				ProcessedCount++;
			}
		}
	}
}

void UUKNPCUpdateManager::RebalanceGroups()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(NPC_Rebalance);

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsValid(PlayerPawn))
	{
		return;
	}

	const FVector PlayerLoc = PlayerPawn->GetActorLocation();

	// 1. 거리 갱신 (마스터 리스트만 순회)
	// 포인터 리스트를 새로 만들기 위해 기존 리스트 클리어
	HighPriorityGroup.Reset();
	LowPriorityGroup.Reset();

	// 정렬을 위한 임시 배열
	TArray<FManagedActorData*> TempSorters;
	TempSorters.Reserve(MasterList.Num());

	for (const auto& DataPtr : MasterList)
	{
		// 스마트 포인터 유효성 체크
		if (DataPtr.IsValid() && IsValid(DataPtr->OwnerActor))
		{
			DataPtr->Distance = FVector::Distance(PlayerLoc, DataPtr->OwnerActor->GetActorLocation());
			TempSorters.Emplace(DataPtr.Get());
		}
	}

	// 2. 거리순 정렬 (오름차순: 가까운 순)
	Algo::Sort(TempSorters, [](const FManagedActorData* A, const FManagedActorData* B)
	{
		return A->Distance < B->Distance;
	});

	// 3. 그룹 분배
	for (int32 i = 0; i < TempSorters.Num(); ++i)
	{
		if (i < HighPriorityCount)
		{
			// Top N(10)
			HighPriorityGroup.Add(TempSorters[i]);
		}
		else
		{
			// 나머지
			LowPriorityGroup.Add(TempSorters[i]);
		}
	}

	// 커서 안전 장치 (리스트 크기가 줄어들었을 경우 대비)
	if (LowPriorityGroup.Num() > 0)
	{
		LowGroupCursor = LowGroupCursor % LowPriorityGroup.Num();
	}
	else
	{
		LowGroupCursor = 0;
	}
}