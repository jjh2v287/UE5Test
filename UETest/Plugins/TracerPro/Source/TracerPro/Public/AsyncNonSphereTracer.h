// Copyright 2024, Cubeap Electronics, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "TracerProMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncNonSphereTracer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAsyncTracePin, const TArray<FHitResult>&, OutHits);
/**
 * 
 */
UCLASS()
class TRACERPRO_API UAsyncNonSphereTracer : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable)
	FAsyncTracePin Completed;
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject", AutoCreateRefTerm="ActorsToIgnore,ObjectTypes"), Category="TracerPro")
	static UAsyncNonSphereTracer* AsyncNonSphereTrace(const UObject* WorldContextObject, EFigureType FigureType, bool TraceComplex,
							  FVector Start, float Width, float Height, float Length, FRotator Angle, EMethod TraceMethod, int32 Value,
							  EModificator Modificator, int32 MaxCount, const TArray<AActor*> ActorsToIgnore, ETraceType TraceType,
							  ETraceTypeQuery CollisionChannel, const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes, FName ProfileName, bool Visualize);

protected:
	
	virtual void Activate() override;

	virtual UWorld* GetWorld() const override
	{
		return IsValid(WorldContextObject) ? WorldContextObject->GetWorld() : nullptr;
	}
	
private:
	
	TArray<FHitResult> TraceResults;
	
	TArray<TArray<FVector>> FixedStep();
	TArray<TArray<FVector>> FixedCount();


	ETraceType TraceType;

	int32 Step;
	float MinimalDistance;
	float Length;
	FVector Start;
	FRotator Angle;
	
	TArray<FVector2D> Diameters;
	EMethod TraceMethod;
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
	
	bool SquareTest(const FVector2D& Test, const FVector2D& D) const {return true;}
	bool CircleTest(const FVector2D& Test, const FVector2D& D) const
	{
		return FVector2D::Distance(FVector2D{Test.X,Test.Y * (D.Y != 0 ? D.X/D.Y : 0)},FVector2D{0}) <= D.X/2;
	}

	bool (UAsyncNonSphereTracer::*Testing)(const FVector2D& Test, const FVector2D& D) const = &UAsyncNonSphereTracer::SquareTest;

	FVector SquareRandom()
	{
		const float W = Diameters[1].X/2;
		const float H = Diameters[1].Y/2;
		return FVector{0, FMath::RandRange(-W,W), FMath::RandRange(-H,H)};
	}
	FVector CircleRandom()
	{
		const float R = FMath::RandRange(0.f, 1.f);
		const float A = FMath::RandRange(0.f, 6.28f);
		const float Y = FMath::Sqrt(R) * FMath::Cos(A);
		const float Z = FMath::Sqrt(R) * FMath::Sin(A);
		return FVector{0, Y*Diameters[1].X/2, Z*Diameters[1].Y/2};
	}

	TArray<FVector> StepGenerate(const FVector2D& D, const FVector& Add);

	FVector (UAsyncNonSphereTracer::*Rand)() = &UAsyncNonSphereTracer::SquareRandom;
	TArray<TArray<FVector>> (UAsyncNonSphereTracer::*Method)() = &UAsyncNonSphereTracer::FixedStep;
};
