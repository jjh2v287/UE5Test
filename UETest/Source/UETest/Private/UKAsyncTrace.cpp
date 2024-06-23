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
		const EAsyncTraceType AsyncTraceType = ConvertToAsyncTraceType(AsyncTraceInfo.AsyncTraceType);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncLineTraceByChannel), AsyncTraceInfo.bTraceComplex);
		Params.AddIgnoredActors(AsyncTraceInfo.InIgnoreActors);
		
		TraceTaskID = GetWorld()->AsyncLineTraceByChannel(
			AsyncTraceType,
			AsyncTraceInfo.StartLoaction,
			AsyncTraceInfo.EndLoaction,
			AsyncTraceInfo.CollisionChannel,
			Params,
			FCollisionResponseParams::DefaultResponseParam,
			&TraceDelegate);
	}
	else
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

UUKAsyncTrace* UUKAsyncTrace::UKAsyncLineTraceByChannel(const UObject* WorldContextObject, const FUKAsyncTraceInfo AsyncTraceInfo)
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
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}

UUKAsyncTrace* UUKAsyncTrace::UKAsyncSweepByChannel(const UObject* WorldContextObject, const FUKAsyncSweepInfo AsyncSweepInfo)
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
