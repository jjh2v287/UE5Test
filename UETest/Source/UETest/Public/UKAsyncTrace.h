// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "UKAsyncTrace.generated.h"

USTRUCT(BlueprintType)
struct FUKAsyncTraceResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = AsyncTraceResult)
	TArray<FHitResult> OutHits;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTracePin, const FUKAsyncTraceResult&, AsyncTraceResult);

/**
 * 
 */
UCLASS()
class UETEST_API UUKAsyncTrace : public UCancellableAsyncAction
{
	GENERATED_BODY()
protected:
	virtual void Activate() override;
	virtual void Cancel() override;
	virtual void BeginDestroy() override;

	virtual UWorld* GetWorld() const override
	{
		return WorldContextObject.IsValid() ? WorldContextObject.Get() : nullptr;
	}

	void AsyncTraceFinish(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UUKAsyncTrace* UKAsyncTrace(const UObject* WorldContextObject, const FVector StartLoaction, const FVector EndLoaction);

private:
	FTraceHandle TraceTaskID; 
	TWeakObjectPtr<UWorld> WorldContextObject = nullptr;
	FTraceDelegate TraceDelegate;

	FVector StartLoaction = FVector::ZeroVector;
	FVector EndLoaction= FVector::ZeroVector;
	
public:
	UPROPERTY(BlueprintAssignable)
	FTracePin FinishPin;
};
