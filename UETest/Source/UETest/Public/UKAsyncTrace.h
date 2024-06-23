// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UKAsyncOverlap.h"
#include "Engine/CancellableAsyncAction.h"
#include "UKAsyncTrace.generated.h"

UENUM(BlueprintType)
enum class EUKAsyncTraceType : uint8
{
	Test,
	Single,
	Multi,
};

USTRUCT(BlueprintType)
struct UETEST_API FUKAsyncTraceInfo
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUKAsyncTraceType AsyncTraceType = EUKAsyncTraceType::Test;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector StartLoaction = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector EndLoaction = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ECollisionChannel> CollisionChannel = ECC_Visibility;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTraceComplex = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<const AActor*> InIgnoreActors;
};

USTRUCT(BlueprintType)
struct UETEST_API FUKAsyncSweepInfo
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUKAsyncTraceType AsyncTraceType = EUKAsyncTraceType::Test;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector StartLoaction = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector EndLoaction = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Rotator = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ECollisionChannel> CollisionChannel = ECC_Visibility;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<const AActor*> InIgnoreActors;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUKAsyncShapeType ShapeType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ShapeType == EAsyncOverLapShape::Box", EditConditionHides))
	FVector BoxExtent = FVector::ZeroVector;
	/*
	Capsule.Radius = FMath::Max(Extent.X, Extent.Y);
	Capsule.HalfHeight = Extent.Z;
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ShapeType == EAsyncOverLapShape::Capsule", EditConditionHides))
	FVector CapsuleExtent = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ShapeType == EAsyncOverLapShape::Sphere", EditConditionHides))
	float SphereRadius = 0.0f;
};

USTRUCT(BlueprintType)
struct FUKAsyncTraceResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AsyncTraceResult)
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
	static UUKAsyncTrace* UKAsyncLineTraceByChannel(const UObject* WorldContextObject, const FUKAsyncTraceInfo AsyncTraceInfo);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UUKAsyncTrace* UKAsyncSweepByChannel(const UObject* WorldContextObject, const FUKAsyncSweepInfo AsyncSweepInfo);

	static const EAsyncTraceType ConvertToAsyncTraceType(const EUKAsyncTraceType Type);
	
private:
	FTraceHandle TraceTaskID; 
	TWeakObjectPtr<UWorld> WorldContextObject = nullptr;
	FTraceDelegate TraceDelegate;

	FUKAsyncTraceInfo AsyncTraceInfo;
	FUKAsyncSweepInfo AsyncSweepInfo;
	
public:
	bool bIsSweep = false;
	
	UPROPERTY(BlueprintAssignable)
	FTracePin FinishPin;
};
