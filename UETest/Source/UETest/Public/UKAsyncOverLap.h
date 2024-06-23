// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "UKAsyncOverlap.generated.h"

enum class EUKExecuteType : uint8
{
	Channel,
	Profile,
	ObjectType,
};

UENUM(BlueprintType)
enum class EUKAsyncShapeType : uint8
{
	Sphere,
	Capsule,
	Box,
};

USTRUCT(BlueprintType)
struct UETEST_API FUKAsyncOverlapInfo
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector OverLapLoaction = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator OverLapRotator = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<const AActor*> InIgnoreActors;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTraceComplex = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUKAsyncShapeType ShapeType = EUKAsyncShapeType::Sphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ShapeType == EAsyncOverLapShape::Box", EditConditionHides))
	FVector BoxExtent = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Radius = Max(Extent.X, Extent.Y) And	HalfHeight = Extent.Z", EditCondition = "ShapeType == EAsyncOverLapShape::Capsule", EditConditionHides))
    FVector CapsuleExtent = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ShapeType == EAsyncOverLapShape::Sphere", EditConditionHides))
	float SphereRadius = 0.0f;
};

USTRUCT(BlueprintType)
struct UETEST_API FUkOverlapResult
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	AActor* OverlapActor;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UPrimitiveComponent* Component;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ItemIndex = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint32 bBlockingHit : 1;

	Chaos::FPhysicsObjectHandle PhysicsObject;
};

USTRUCT(BlueprintType)
struct UETEST_API FUKAsyncOverlapResult
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector Loction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator	Rotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TEnumAsByte<ECollisionChannel> TraceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FUkOverlapResult> OutOverlaps;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOverLapPin, const FUKAsyncOverlapResult&, AsyncTraceResult);

/**
 * 
 */
UCLASS()
class UETEST_API UUKAsyncOverlap : public UCancellableAsyncAction
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
	static UUKAsyncOverlap* UKAsyncOverlapByChannel(const UObject* WorldContextObject, const ECollisionChannel CollisionChannel, const FUKAsyncOverlapInfo AsyncOverLapInfo);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UUKAsyncOverlap* UKAsyncOverlapByProfile(const UObject* WorldContextObject, const FName ProfileName, const FUKAsyncOverlapInfo AsyncOverLapInfo);
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UUKAsyncOverlap* UKAsyncOverlapByObjectType(const UObject* WorldContextObject, const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes, const FUKAsyncOverlapInfo AsyncOverLapInfo);

	static const FCollisionShape MakeCollisionShape(const EUKAsyncShapeType Type, const FVector BoxExtent, const FVector CapsuleExtent, const float SphereRadius);

private:
	void ExecuteAsyncOverlapByChannel();
	void ExecuteAsyncOverlapByProfile();
	void ExecuteAsyncOverlapByObjectType();
	
private:
	FTraceHandle TraceTaskID; 
	TWeakObjectPtr<UWorld> WorldContextObject = nullptr;
	FOverlapDelegate OverlapDelegate;

	FUKAsyncOverlapInfo AsyncOverLapInfo;

	EUKExecuteType ExecuteType = EUKExecuteType::Channel;
	ECollisionChannel CollisionChannel = ECC_WorldStatic;
	FName ProfileName = NAME_None;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	
public:
	UPROPERTY(BlueprintAssignable)
	FOverLapPin FinishPin;
};
