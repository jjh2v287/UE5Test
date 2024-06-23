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
	
	if(ExecuteType == EUKExecuteType::Channel)
	{
		ExecuteAsyncOverlapByChannel();
	}
	else if(ExecuteType == EUKExecuteType::Profile)
	{
		ExecuteAsyncOverlapByProfile();
	}
	else if(ExecuteType == EUKExecuteType::ObjectType)
	{
		ExecuteAsyncOverlapByObjectType();
	}
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

UUKAsyncOverlap* UUKAsyncOverlap::UKAsyncOverlapByChannel(const UObject* WorldContextObject, const ECollisionChannel CollisionChannel, const FUKAsyncOverlapInfo AsyncOverLapInfo)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UUKAsyncOverlap* Node = NewObject<UUKAsyncOverlap>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->AsyncOverLapInfo = AsyncOverLapInfo;
	Node->CollisionChannel = CollisionChannel;
	Node->ExecuteType = EUKExecuteType::Channel;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}

UUKAsyncOverlap* UUKAsyncOverlap::UKAsyncOverlapByProfile(const UObject* WorldContextObject, const FName ProfileName, const FUKAsyncOverlapInfo AsyncOverLapInfo)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UUKAsyncOverlap* Node = NewObject<UUKAsyncOverlap>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->AsyncOverLapInfo = AsyncOverLapInfo;
	Node->ProfileName = ProfileName;
	Node->ExecuteType = EUKExecuteType::Profile;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}

UUKAsyncOverlap* UUKAsyncOverlap::UKAsyncOverlapByObjectType(const UObject* WorldContextObject, const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes, const FUKAsyncOverlapInfo AsyncOverLapInfo)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
    {
    	return nullptr;
    }
    
    UUKAsyncOverlap* Node = NewObject<UUKAsyncOverlap>();
    Node->WorldContextObject = WorldContextObject->GetWorld();
    Node->AsyncOverLapInfo = AsyncOverLapInfo;
    Node->ObjectTypes = ObjectTypes;
	Node->ExecuteType = EUKExecuteType::ObjectType;
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

void UUKAsyncOverlap::ExecuteAsyncOverlapByChannel()
{
	const FCollisionShape CollisionShape = MakeCollisionShape(AsyncOverLapInfo.ShapeType, AsyncOverLapInfo.BoxExtent, AsyncOverLapInfo.CapsuleExtent, AsyncOverLapInfo.SphereRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncOverLap), AsyncOverLapInfo.bTraceComplex);
	Params.AddIgnoredActors(MoveTemp(AsyncOverLapInfo.InIgnoreActors));

	TraceTaskID = GetWorld()->AsyncOverlapByChannel(
		AsyncOverLapInfo.OverLapLoaction,
		AsyncOverLapInfo.OverLapRotator.Quaternion(),
		CollisionChannel,
		CollisionShape,
		Params,
		FCollisionResponseParams::DefaultResponseParam,
		&OverlapDelegate);
}

void UUKAsyncOverlap::ExecuteAsyncOverlapByProfile()
{
	const FCollisionShape CollisionShape = MakeCollisionShape(AsyncOverLapInfo.ShapeType, AsyncOverLapInfo.BoxExtent, AsyncOverLapInfo.CapsuleExtent, AsyncOverLapInfo.SphereRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncOverLap), AsyncOverLapInfo.bTraceComplex);
	Params.AddIgnoredActors(MoveTemp(AsyncOverLapInfo.InIgnoreActors));

	TraceTaskID = GetWorld()->AsyncOverlapByProfile(
		AsyncOverLapInfo.OverLapLoaction,
		AsyncOverLapInfo.OverLapRotator.Quaternion(),
		ProfileName,
		CollisionShape,
		Params,
		&OverlapDelegate);
}

void UUKAsyncOverlap::ExecuteAsyncOverlapByObjectType()
{
	const FCollisionObjectQueryParams ObjectQueryParams (ObjectTypes);
	const FCollisionShape CollisionShape = MakeCollisionShape(AsyncOverLapInfo.ShapeType, AsyncOverLapInfo.BoxExtent, AsyncOverLapInfo.CapsuleExtent, AsyncOverLapInfo.SphereRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncOverLap), AsyncOverLapInfo.bTraceComplex);
	Params.AddIgnoredActors(MoveTemp(AsyncOverLapInfo.InIgnoreActors));

	TraceTaskID = GetWorld()->AsyncOverlapByObjectType(
		AsyncOverLapInfo.OverLapLoaction,
		AsyncOverLapInfo.OverLapRotator.Quaternion(),
		ObjectQueryParams,
		CollisionShape,
		Params,
		&OverlapDelegate);
}
