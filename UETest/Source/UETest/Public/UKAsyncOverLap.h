// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "UKAsyncOverLap.generated.h"

UENUM(BlueprintType)
enum class EAsyncOverLapShape : uint8
{
	Line,
	Box,
	Sphere,
	Capsule
};

USTRUCT(BlueprintType)
struct UETEST_API FUKAsyncOverLapInfo
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	FVector OverLapLoaction = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite)
	FRotator OverLapRotator = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadWrite)
	TEnumAsByte<ECollisionChannel> CollisionChannel;
	UPROPERTY(BlueprintReadWrite)
	TArray<const AActor*> InIgnoreActors;
	UPROPERTY(BlueprintReadWrite)
	EAsyncOverLapShape ShapeType;
};

USTRUCT(BlueprintType)
struct UETEST_API FUkOverlapResult
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	AActor* OverlapActor;
	UPROPERTY(BlueprintReadOnly)
	UPrimitiveComponent* Component;
	UPROPERTY(BlueprintReadOnly)
	int32 ItemIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly)
	uint32 bBlockingHit : 1;

	Chaos::FPhysicsObjectHandle PhysicsObject;
};

USTRUCT(BlueprintType)
struct UETEST_API FUKAsyncOverLapResult
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	FVector Loction;
	UPROPERTY(BlueprintReadOnly)
	FRotator	Rotator;

	UPROPERTY(BlueprintReadOnly)
	TEnumAsByte<ECollisionChannel> TraceChannel;

	UPROPERTY(BlueprintReadOnly)
	TArray<FUkOverlapResult> OutOverlaps;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOverLapPin, const FUKAsyncOverLapResult&, AsyncTraceResult);

/**
 * 
 */
UCLASS()
class UETEST_API UUKAsyncOverLap : public UCancellableAsyncAction
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

	void AsyncOverLapFinish(const FTraceHandle& TraceHandle, FOverlapDatum& OverlapDatum);

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UUKAsyncOverLap* UKAsyncOverLap(const UObject* WorldContextObject, const FUKAsyncOverLapInfo AsyncOverLapInfo);

private:
	FTraceHandle TraceTaskID; 
	TWeakObjectPtr<UWorld> WorldContextObject = nullptr;
	FOverlapDelegate OverlapDelegate;

	FUKAsyncOverLapInfo AsyncOverLapInfo;
	
public:
	UPROPERTY(BlueprintAssignable)
	FOverLapPin FinishPin;
};
