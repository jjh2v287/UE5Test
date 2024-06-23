// Copyright Kong Studios, Inc. All Rights Reserved.

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKAsyncTrace)

#include "UKAsyncTrace.h"

void UUKAsyncTrace::Activate()
{
	Super::Activate();

	if (!TraceDelegate.IsBound())
	{
		TraceDelegate.BindUObject(this, &UUKAsyncTrace::AsyncTraceFinish);
	}
	
	if(!bIsSweep)
	{
		if(ExecuteType == EUKExecuteType::Channel)
		{
			ExecuteAsyncLineTraceByChannel();
		}
		else if(ExecuteType == EUKExecuteType::Profile)
		{
			ExecuteAsyncLineTraceByProfile();
		}
		else if(ExecuteType == EUKExecuteType::ObjectType)
		{
			ExecuteAsyncLineTraceByObjectType();
		}
	}
	else
	{
		if(ExecuteType == EUKExecuteType::Channel)
        {
        	ExecuteAsyncSweepByChannel();
        }
        else if(ExecuteType == EUKExecuteType::Profile)
        {
        	ExecuteAsyncSweepByProfile();
        }
        else if(ExecuteType == EUKExecuteType::ObjectType)
        {
        	ExecuteAsyncSweepByObjectType();
        }
	}
}

void UUKAsyncTrace::Cancel()
{
	Super::Cancel();
	if (TraceDelegate.IsBound())
    {
    	TraceDelegate.Unbind();
    }
}

void UUKAsyncTrace::BeginDestroy()
{
	Super::BeginDestroy();

	if (TraceDelegate.IsBound())
	{
		TraceDelegate.Unbind();
	}
}

void UUKAsyncTrace::AsyncTraceFinish(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum)
{
	if(TraceTaskID != TraceHandle)
	{
		return;
	}

	FUKAsyncTraceResult AsyncTraceResult;
	AsyncTraceResult.OutHits = MoveTemp(TraceDatum.OutHits);
	FinishPin.Broadcast(AsyncTraceResult);
}

UUKAsyncTrace* UUKAsyncTrace::UKAsyncLineTraceByChannel(const UObject* WorldContextObject, const ECollisionChannel CollisionChannel, const FUKAsyncTraceInfo AsyncTraceInfo)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UUKAsyncTrace* Node = NewObject<UUKAsyncTrace>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->AsyncTraceInfo = AsyncTraceInfo;
	Node->bIsSweep = false;
	Node->CollisionChannel = CollisionChannel;
	Node->ExecuteType = EUKExecuteType::Channel;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}

UUKAsyncTrace* UUKAsyncTrace::UKAsyncLineTraceByProfile(const UObject* WorldContextObject, const FName ProfileName, const FUKAsyncTraceInfo AsyncTraceInfo)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UUKAsyncTrace* Node = NewObject<UUKAsyncTrace>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->AsyncTraceInfo = AsyncTraceInfo;
	Node->bIsSweep = false;
	Node->ProfileName = ProfileName;
	Node->ExecuteType = EUKExecuteType::Profile;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}

UUKAsyncTrace* UUKAsyncTrace::UKAsyncLineTraceByObjectType(const UObject* WorldContextObject, const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes, const FUKAsyncTraceInfo AsyncTraceInfo)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UUKAsyncTrace* Node = NewObject<UUKAsyncTrace>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->AsyncTraceInfo = AsyncTraceInfo;
	Node->bIsSweep = false;
	Node->ObjectTypes = ObjectTypes;
	Node->ExecuteType = EUKExecuteType::ObjectType;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}

UUKAsyncTrace* UUKAsyncTrace::UKAsyncSweepByChannel(const UObject* WorldContextObject, const ECollisionChannel CollisionChannel, const FUKAsyncSweepInfo AsyncSweepInfo)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UUKAsyncTrace* Node = NewObject<UUKAsyncTrace>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->AsyncSweepInfo = AsyncSweepInfo;
	Node->bIsSweep = true;
	Node->CollisionChannel = CollisionChannel;
	Node->ExecuteType = EUKExecuteType::Channel;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}

UUKAsyncTrace* UUKAsyncTrace::UKAsyncSweepByProfile(const UObject* WorldContextObject, const FName ProfileName, const FUKAsyncSweepInfo AsyncSweepInfo)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UUKAsyncTrace* Node = NewObject<UUKAsyncTrace>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->AsyncSweepInfo = AsyncSweepInfo;
	Node->bIsSweep = true;
	Node->ProfileName = ProfileName;
	Node->ExecuteType = EUKExecuteType::Profile;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}

UUKAsyncTrace* UUKAsyncTrace::UKAsyncSweepByObjectType(const UObject* WorldContextObject, const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes, const FUKAsyncSweepInfo AsyncSweepInfo)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UUKAsyncTrace* Node = NewObject<UUKAsyncTrace>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->AsyncSweepInfo = AsyncSweepInfo;
	Node->bIsSweep = true;
	Node->ObjectTypes = ObjectTypes;
	Node->ExecuteType = EUKExecuteType::ObjectType;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}

const EAsyncTraceType UUKAsyncTrace::ConvertToAsyncTraceType(const EUKAsyncTraceType Type)
{
	EAsyncTraceType AsyncTraceType = EAsyncTraceType::Test;
	
	if(Type == EUKAsyncTraceType::Single)
	{
		AsyncTraceType = EAsyncTraceType::Single;
	}
	else if(Type == EUKAsyncTraceType::Multi)
	{
		AsyncTraceType = EAsyncTraceType::Multi;
	}

	return AsyncTraceType;
}

void UUKAsyncTrace::ExecuteAsyncLineTraceByChannel()
{
	const EAsyncTraceType AsyncTraceType = ConvertToAsyncTraceType(AsyncTraceInfo.AsyncTraceType);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncLineTraceByChannel), AsyncTraceInfo.bTraceComplex);
	Params.AddIgnoredActors(AsyncTraceInfo.InIgnoreActors);

	TraceTaskID = GetWorld()->AsyncLineTraceByChannel(
		AsyncTraceType,
		AsyncTraceInfo.StartLoaction,
		AsyncTraceInfo.EndLoaction,
		CollisionChannel,
		Params,
		FCollisionResponseParams::DefaultResponseParam,
		&TraceDelegate);
}

void UUKAsyncTrace::ExecuteAsyncLineTraceByProfile()
{
	const EAsyncTraceType AsyncTraceType = ConvertToAsyncTraceType(AsyncTraceInfo.AsyncTraceType);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncLineTraceByProfile), AsyncTraceInfo.bTraceComplex);
	Params.AddIgnoredActors(AsyncTraceInfo.InIgnoreActors);

	TraceTaskID = GetWorld()->AsyncLineTraceByProfile(
		AsyncTraceType,
		AsyncTraceInfo.StartLoaction,
		AsyncTraceInfo.EndLoaction,
		ProfileName,
		Params,
		&TraceDelegate);
}

void UUKAsyncTrace::ExecuteAsyncLineTraceByObjectType()
{
	const FCollisionObjectQueryParams ObjectQueryParams (ObjectTypes);
	const EAsyncTraceType AsyncTraceType = ConvertToAsyncTraceType(AsyncTraceInfo.AsyncTraceType);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncLineTraceByObjectType), AsyncTraceInfo.bTraceComplex);
	Params.AddIgnoredActors(AsyncTraceInfo.InIgnoreActors);
	
	TraceTaskID = GetWorld()->AsyncLineTraceByObjectType(
		AsyncTraceType,
		AsyncTraceInfo.StartLoaction,
		AsyncTraceInfo.EndLoaction,
		ObjectQueryParams,
		Params,
		&TraceDelegate);
}

void UUKAsyncTrace::ExecuteAsyncSweepByChannel()
{
	const EAsyncTraceType AsyncTraceType = ConvertToAsyncTraceType(AsyncSweepInfo.AsyncTraceType);
	const FCollisionShape CollisionShape = UUKAsyncOverlap::MakeCollisionShape(AsyncSweepInfo.ShapeType, AsyncSweepInfo.BoxExtent, AsyncSweepInfo.CapsuleExtent, AsyncSweepInfo.SphereRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncSweepByChannel), AsyncTraceInfo.bTraceComplex);
	Params.AddIgnoredActors(AsyncSweepInfo.InIgnoreActors);

	TraceTaskID = GetWorld()->AsyncSweepByChannel(
		AsyncTraceType,
		AsyncSweepInfo.StartLoaction,
		AsyncSweepInfo.EndLoaction,
		AsyncSweepInfo.Rotator.Quaternion(),
		AsyncSweepInfo.CollisionChannel,
		CollisionShape,
		Params,
		FCollisionResponseParams::DefaultResponseParam,
		&TraceDelegate);
}

void UUKAsyncTrace::ExecuteAsyncSweepByProfile()
{
	const EAsyncTraceType AsyncTraceType = ConvertToAsyncTraceType(AsyncSweepInfo.AsyncTraceType);
	const FCollisionShape CollisionShape = UUKAsyncOverlap::MakeCollisionShape(AsyncSweepInfo.ShapeType, AsyncSweepInfo.BoxExtent, AsyncSweepInfo.CapsuleExtent, AsyncSweepInfo.SphereRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncSweepByProfile), AsyncTraceInfo.bTraceComplex);
	Params.AddIgnoredActors(AsyncSweepInfo.InIgnoreActors);

	TraceTaskID = GetWorld()->AsyncSweepByProfile(
		AsyncTraceType,
		AsyncSweepInfo.StartLoaction,
		AsyncSweepInfo.EndLoaction,
		AsyncSweepInfo.Rotator.Quaternion(),
		ProfileName,
		CollisionShape,
		Params,
		&TraceDelegate);
}

void UUKAsyncTrace::ExecuteAsyncSweepByObjectType()
{
	const FCollisionObjectQueryParams ObjectQueryParams (ObjectTypes);
	const EAsyncTraceType AsyncTraceType = ConvertToAsyncTraceType(AsyncSweepInfo.AsyncTraceType);
	const FCollisionShape CollisionShape = UUKAsyncOverlap::MakeCollisionShape(AsyncSweepInfo.ShapeType, AsyncSweepInfo.BoxExtent, AsyncSweepInfo.CapsuleExtent, AsyncSweepInfo.SphereRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncSweepByObjectType), AsyncTraceInfo.bTraceComplex);
	Params.AddIgnoredActors(AsyncSweepInfo.InIgnoreActors);

	TraceTaskID = GetWorld()->AsyncSweepByObjectType(
		AsyncTraceType,
		AsyncSweepInfo.StartLoaction,
		AsyncSweepInfo.EndLoaction,
		AsyncSweepInfo.Rotator.Quaternion(),
		ObjectQueryParams,
		CollisionShape,
		Params,
		&TraceDelegate);
}
