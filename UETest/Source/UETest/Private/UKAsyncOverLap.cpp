// Copyright Kong Studios, Inc. All Rights Reserved.

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKAsyncOverLap)

#include "UKAsyncOverLap.h"

#include "Engine/OverlapResult.h"

void UUKAsyncOverLap::Activate()
{
	Super::Activate();

	if (!OverlapDelegate.IsBound())
	{
		OverlapDelegate.BindUObject(this, &UUKAsyncOverLap::AsyncOverLapFinish);
	}

	FCollisionShape CollisionShape;
	switch (AsyncOverLapInfo.ShapeType)
	{
	case EAsyncOverLapShape::Line:
		CollisionShape.ShapeType = ECollisionShape::Type::Line;
		break;
	case EAsyncOverLapShape::Box:
		CollisionShape.ShapeType = ECollisionShape::Type::Box;
		break;
	case EAsyncOverLapShape::Capsule:
		CollisionShape.ShapeType = ECollisionShape::Type::Capsule;
		break;
	case EAsyncOverLapShape::Sphere:
		CollisionShape.ShapeType = ECollisionShape::Type::Sphere;
		CollisionShape.SetSphere(100.0f);
		break;
	default:
		CollisionShape.ShapeType = ECollisionShape::Type::Line;
		break;
	}
	
	
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncOverLap), true);
	Params.AddIgnoredActors(MoveTemp(AsyncOverLapInfo.InIgnoreActors));
	
	TraceTaskID = GetWorld()->AsyncOverlapByChannel(
		AsyncOverLapInfo.OverLapLoaction,
		AsyncOverLapInfo.OverLapRotator.Quaternion(),
		AsyncOverLapInfo.CollisionChannel,
		CollisionShape,
		Params,
		FCollisionResponseParams::DefaultResponseParam,
		&OverlapDelegate);
}

void UUKAsyncOverLap::Cancel()
{
	Super::Cancel();
	if (OverlapDelegate.IsBound())
    {
    	OverlapDelegate.Unbind();
    }
}

void UUKAsyncOverLap::BeginDestroy()
{
	Super::BeginDestroy();

	if (OverlapDelegate.IsBound())
	{
		OverlapDelegate.Unbind();
	}
}

void UUKAsyncOverLap::AsyncOverLapFinish(const FTraceHandle& TraceHandle, FOverlapDatum& OverlapDatum)
{
	if(TraceTaskID != TraceHandle)
	{
		return;
	}

	FUKAsyncOverLapResult AsyncOverLapResult;
	AsyncOverLapResult.Loction = OverlapDatum.Pos;
	AsyncOverLapResult.Rotator = OverlapDatum.Rot.Rotator();
	AsyncOverLapResult.TraceChannel = OverlapDatum.TraceChannel;
	for(const FOverlapResult& Result : OverlapDatum.OutOverlaps)
	{
		FUkOverlapResult UkOverlapResult;
		UkOverlapResult.OverlapActor = Result.GetActor();
		UkOverlapResult.Component = Result.GetComponent();
		UkOverlapResult.bBlockingHit = Result.bBlockingHit;
		UkOverlapResult.ItemIndex = Result.ItemIndex;
		UkOverlapResult.PhysicsObject = Result.PhysicsObject;
		
		AsyncOverLapResult.OutOverlaps.Emplace(UkOverlapResult);
	}
	FinishPin.Broadcast(AsyncOverLapResult);
}

UUKAsyncOverLap* UUKAsyncOverLap::UKAsyncOverLap(const UObject* WorldContextObject, const FUKAsyncOverLapInfo AsyncOverLapInfo)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UUKAsyncOverLap* Node = NewObject<UUKAsyncOverLap>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->AsyncOverLapInfo = AsyncOverLapInfo;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}
