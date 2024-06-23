// Copyright Kong Studios, Inc. All Rights Reserved.

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKAsyncOverLap)

#include "UKAsyncOverlap.h"

#include "Engine/OverlapResult.h"

void UUKAsyncOverlap::Activate()
{
	Super::Activate();

	if (!OverlapDelegate.IsBound())
	{
		OverlapDelegate.BindUObject(this, &UUKAsyncOverlap::AsyncOverLapFinish);
	}

	const FCollisionShape CollisionShape = MakeCollisionShape(AsyncOverLapInfo.ShapeType, AsyncOverLapInfo.BoxExtent, AsyncOverLapInfo.CapsuleExtent, AsyncOverLapInfo.SphereRadius);
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

void UUKAsyncOverlap::Cancel()
{
	Super::Cancel();
	if (OverlapDelegate.IsBound())
    {
    	OverlapDelegate.Unbind();
    }
}

void UUKAsyncOverlap::BeginDestroy()
{
	Super::BeginDestroy();

	if (OverlapDelegate.IsBound())
	{
		OverlapDelegate.Unbind();
	}
}

void UUKAsyncOverlap::AsyncOverLapFinish(const FTraceHandle& TraceHandle, FOverlapDatum& OverlapDatum)
{
	if(TraceTaskID != TraceHandle)
	{
		return;
	}

	FUKAsyncOverlapResult AsyncOverLapResult;
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

UUKAsyncOverlap* UUKAsyncOverlap::UKAsyncOverlapByChannel(const UObject* WorldContextObject, const FUKAsyncOverlapInfo AsyncOverLapInfo)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UUKAsyncOverlap* Node = NewObject<UUKAsyncOverlap>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->AsyncOverLapInfo = AsyncOverLapInfo;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}

const FCollisionShape UUKAsyncOverlap::MakeCollisionShape(const EUKAsyncShapeType Type, const FVector BoxExtent, const FVector CapsuleExtent, const float SphereRadius)
{
	FCollisionShape CollisionShape;
	
	if(Type == EUKAsyncShapeType::Box)
	{
		CollisionShape = FCollisionShape::MakeBox(BoxExtent);
	}
	else if(Type == EUKAsyncShapeType::Capsule)
	{
		CollisionShape = FCollisionShape::MakeCapsule(CapsuleExtent);
	}
	else if(Type == EUKAsyncShapeType::Sphere)
	{
		CollisionShape = FCollisionShape::MakeSphere(SphereRadius);
	}

	return CollisionShape;
}
