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
	TArray<const AActor*> InIgnoreActors;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTraceComplex = false;
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
	TArray<const AActor*> InIgnoreActors;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTraceComplex = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUKAsyncShapeType ShapeType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ShapeType == EAsyncOverLapShape::Box", EditConditionHides))
	FVector BoxExtent = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Radius = Max(Extent.X, Extent.Y) And	HalfHeight = Extent.Z", EditCondition = "ShapeType == EAsyncOverLapShape::Capsule", EditConditionHides))
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
	static UUKAsyncTrace* UKAsyncLineTraceByChannel(const UObject* WorldContextObject, const ECollisionChannel CollisionChannel, const FUKAsyncTraceInfo AsyncTraceInfo);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UUKAsyncTrace* UKAsyncLineTraceByProfile(const UObject* WorldContextObject, const FName ProfileName, const FUKAsyncTraceInfo AsyncTraceInfo);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UUKAsyncTrace* UKAsyncLineTraceByObjectType(const UObject* WorldContextObject, const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes, const FUKAsyncTraceInfo AsyncTraceInfo);
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UUKAsyncTrace* UKAsyncSweepByChannel(const UObject* WorldContextObject, const ECollisionChannel CollisionChannel, const FUKAsyncSweepInfo AsyncSweepInfo);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UUKAsyncTrace* UKAsyncSweepByProfile(const UObject* WorldContextObject, const FName ProfileName, const FUKAsyncSweepInfo AsyncSweepInfo);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UUKAsyncTrace* UKAsyncSweepByObjectType(const UObject* WorldContextObject, const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes, const FUKAsyncSweepInfo AsyncSweepInfo);

	static const EAsyncTraceType ConvertToAsyncTraceType(const EUKAsyncTraceType Type);

private:
	void ExecuteAsyncLineTraceByChannel();
	void ExecuteAsyncLineTraceByProfile();
	void ExecuteAsyncLineTraceByObjectType();
	
	void ExecuteAsyncSweepByChannel();
	void ExecuteAsyncSweepByProfile();
	void ExecuteAsyncSweepByObjectType();
	
private:
	FTraceHandle TraceTaskID; 
	TWeakObjectPtr<UWorld> WorldContextObject = nullptr;
	FTraceDelegate TraceDelegate;

	FUKAsyncTraceInfo AsyncTraceInfo;
	FUKAsyncSweepInfo AsyncSweepInfo;

	EUKExecuteType ExecuteType = EUKExecuteType::Channel;
	ECollisionChannel CollisionChannel;
	FName ProfileName = NAME_None;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	
public:
	bool bIsSweep = false;
	
	UPROPERTY(BlueprintAssignable)
	FTracePin FinishPin;
};
