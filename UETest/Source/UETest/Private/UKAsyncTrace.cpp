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
	
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UKAsyncTrace), true);
	// Params.AddIgnoredActor();
	TraceTaskID = GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Test, StartLoaction, EndLoaction, ECollisionChannel::ECC_Visibility, Params, FCollisionResponseParams::DefaultResponseParam, &TraceDelegate);
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

UUKAsyncTrace* UUKAsyncTrace::UKAsyncTrace(const UObject* WorldContextObject, const FVector StartLoaction, const FVector EndLoaction)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UUKAsyncTrace* Node = NewObject<UUKAsyncTrace>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->StartLoaction = StartLoaction;
	Node->EndLoaction = EndLoaction;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}
