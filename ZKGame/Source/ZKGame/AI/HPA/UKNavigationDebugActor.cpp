// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKNavigationDebugActor.h"
#include "DrawDebugHelpers.h"
#include "UKNavigationManager.h"
#include "UKWayPoint.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKNavigationDebugActor)

AUKNavigationDebugActor::AUKNavigationDebugActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AUKNavigationDebugActor::BeginPlay()
{
	Super::BeginPlay();
}

void AUKNavigationDebugActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 에디터 또는 시뮬레이션 환경에서 실행
#if WITH_EDITOR
	UpdatePathVisualization();
#endif
}

void AUKNavigationDebugActor::UpdatePathVisualization()
{
	UWorld* World = GetWorld();
	if (!World || !EndActor)
	{
		LastFoundPath.Empty();
		return;
	}

	UUKNavigationManager* NavigationManage = World->GetSubsystem<UUKNavigationManager>();
	if (!NavigationManage)
	{
		LastFoundPath.Empty();
		UE_LOG(LogTemp, Error, TEXT("AUKHPADebugActor: Failed to get NavigationManage instance."));
		return;
	}

	// Tick에서 매번 호출하는 것은 매우 비효율적! 테스트 용도로만 남겨둠.
	// 실제로는 BeginPlay나 특정 이벤트에서 한 번만 호출해야 함.
	// 강제 재등록 및 빌드 (테스트용)
	NavigationManage->AllRegisterWaypoint();

	// 경로 찾기 (이제 시작/끝 위치를 직접 전달)
	FVector StartLoc = GetActorLocation();
	FVector EndLoc = EndActor->GetActorLocation();

	// FindPath 호출 및 TObjectPtr로 변환
	TArray<AUKWayPoint*> RawPath = NavigationManage->FindPath(StartLoc, EndLoc);
	LastFoundPath.Empty(RawPath.Num());
	for (AUKWayPoint* WP : RawPath)
	{
		if (WP)
		{
			LastFoundPath.Add(WP);
		}
	}

	if (bDrawHPAStructure)
	{
		NavigationManage->DrawDebugHPA(DebugDrawDuration);

		// 경로 시각화
		DrawDebugSphere(World, StartLoc, DebugSphereRadius, 24, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);
		DrawDebugSphere(World, EndLoc, DebugSphereRadius, 24, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);

		if (LastFoundPath.Num() > 0)
		{
			// 시작점 -> 첫 웨이포인트
			if (LastFoundPath[0].Get())
			{
				DrawDebugDirectionalArrow(World, StartLoc, LastFoundPath[0]->GetActorLocation(), DebugArrowSize, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);
			}

			// 웨이포인트 간 경로
			for (int32 i = 0; i < LastFoundPath.Num() - 1; ++i)
			{
				AUKWayPoint* CurrentWP = LastFoundPath[i].Get();
				AUKWayPoint* NextWP = LastFoundPath[i + 1].Get();
				if (CurrentWP && NextWP)
				{
					DrawDebugDirectionalArrow(World, CurrentWP->GetActorLocation(), NextWP->GetActorLocation(), DebugArrowSize, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);
				}
			}

			// 마지막 웨이포인트 -> 끝점
			AUKWayPoint* LastWP = LastFoundPath.Last().Get();
			if (LastWP)
			{
				DrawDebugDirectionalArrow(World, LastWP->GetActorLocation(), EndLoc, DebugArrowSize, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);
			}
		}
		else
		{
			// 경로 못 찾았을 때 시작->끝 직선 (빨간색) 표시 (옵션)
			DrawDebugLine(World, StartLoc, EndLoc, FColor::Red, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness * 0.5f);
		}
	}
}
