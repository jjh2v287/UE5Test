// Copyright 2024, Cubeap Electronics, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "TracerProMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncSphereTracer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAsyncSphereTracePin, const TArray<FHitResult> &, OutHits);
/**
 * 
 */
UCLASS()
class TRACERPRO_API UAsyncSphereTracer : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintAssignable)
	FAsyncSphereTracePin Completed;
	

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject", AutoCreateRefTerm="ActorsToIgnore,ObjectTypes"), Category="TracerPro")
	static UAsyncSphereTracer* AsyncSphereTrace(const UObject* WorldContextObject, bool TraceComplex, FVector Start, ESphereType SphereType, FRotator Angle, float Radius, ESphereMethod TraceMethod,
		int32 Count, EModificator Modificator, int32 MaxCount, const TArray<AActor*> ActorsToIgnore, ETraceProType TraceType, ETraceTypeQuery CollisionChannel,
		const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes, FName ProfileName, bool Visualize);

protected:
	
	virtual void Activate() override;

	virtual UWorld* GetWorld() const override
	{
		return IsValid(WorldContextObject) ? WorldContextObject->GetWorld() : nullptr;
	}
	
private:

	FVector RandomVector(int32 i, int32 Value) const;
	FVector FixedVector(int32 i, int32 Value) const;
	
	TArray<FHitResult> TraceResults;
	
	FVector Start;
	EFigureType FigureType;
	ETraceProType TraceType;
	FRotator Angle;
	FRotator Default;
	float test = 1;
	int32 Count;
	float MinimalDistance;
	float Radius;
	ESphereMethod TraceMethod;
	EModificator Modificator;
	int32 MaxCount;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	FName ProfileName;
	bool Visualize;
	UPROPERTY()
	UObject* WorldContextObject;
	
	TrType CurrentTrace;

	ETraceTypeQuery CollisionChannel;
	FCollisionQueryParams CollisionParams;

	TArray<TArray<FVector>> RunSphere();
	FVector (UAsyncSphereTracer::*GetSphereVector)(int32 i, int32 Value) const = &UAsyncSphereTracer::FixedVector;
};
