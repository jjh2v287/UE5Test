// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKWayPoint.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "NavigationInvokerComponent.h"
#include "UKNavigationManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKWayPoint)

AUKWayPoint::AUKWayPoint(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    InvokerComponent = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavigationInvoker"));
}

void AUKWayPoint::BeginPlay()
{
    Super::BeginPlay();
    
    UWorld* World = GetWorld();
    if (World && World->IsGameWorld())
    {
        if (UUKNavigationManager* NavigationManage = World->GetSubsystem<UUKNavigationManager>())
        {
           WayPointHandle = NavigationManage->RegisterWaypoint(this);
        }
    }
}

void AUKWayPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    const UWorld* World = GetWorld();
    if (World && World->IsGameWorld() && UUKNavigationManager::Get())
    {
        if (UUKNavigationManager* NavigationManage = World->GetSubsystem<UUKNavigationManager>())
        {
            NavigationManage->UnregisterWaypoint(this);
        }
    }
}

void AUKWayPoint::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

#if WITH_EDITOR
void AUKWayPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();

        if (PropertyName == GET_MEMBER_NAME_CHECKED(AUKWayPoint, PathPoints))
        {
            for (AUKWayPoint* NextPoint : PathPoints)
            {
                if (NextPoint && !NextPoint->PathPoints.Contains(this))
                {
                    NextPoint->Modify();
                    NextPoint->PathPoints.Add(this);
                    NextPoint->PostEditChange();
                }
            }
        }
    }
}
#endif