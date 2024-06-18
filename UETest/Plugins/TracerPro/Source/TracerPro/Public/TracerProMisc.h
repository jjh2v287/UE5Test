// Copyright 2024, Cubeap Electronics, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Containers/Array.h"
#include "Engine/HitResult.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "Async/ParallelFor.h"
#include "Engine/World.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "TracerProMisc.generated.h"

UENUM(BlueprintType)
enum class EFigureType : uint8 {
	TrPyramid = 0	UMETA(DisplayName="Pyramid"),
	TrCube			UMETA(DisplayName="Rectangle"),
	TrCone			UMETA(DisplayName="Cone"),
	TrCylinder		UMETA(DisplayName="Cylinder")
};

UENUM(BlueprintType)
enum class ETraceType : uint8 {
	TrChannel = 0	UMETA(DisplayName="By Channel"),
	TrProfile		UMETA(DisplayName="By Profile"),
	TrObject		UMETA(DisplayName="By Object"),
};

UENUM(BlueprintType)
enum class EMethod : uint8{
	TrFixedStep = 0	 UMETA(DisplayName="Fixed Step"),
	TrFixedCount	 UMETA(DisplayName="Fixed Count")
};

UENUM(BlueprintType)
enum class ESphereMethod : uint8{
	TrFixedDistance = 0	 UMETA(DisplayName="Fixed Distance"),
	TrRandomDistance	 UMETA(DisplayName="Random Distance")
};

UENUM(BlueprintType)
enum class ESphereType : uint8{
	TrSphere = 0		 UMETA(DisplayName="Sphere"),
	TrHemiSphere = 1	 UMETA(DisplayName="Hemisphere")
};

UENUM(BlueprintType)
enum class EModificator : uint8{
	TrNone = 0   UMETA(DisplayName="None"),
	TrMultiLine	 UMETA(DisplayName="Multi Line"),
	TrBounce	 UMETA(DisplayName="Bounce")
};

USTRUCT(BlueprintType)
struct FHitsActor
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, Category="TracerPro")
	AActor* Actor;
	UPROPERTY(BlueprintReadWrite, Category="TracerPro")
	int32 HitsCount;
};

typedef bool (*TrType) (TArray<FHitResult>&, const UE::Math::TVector<double>&, const UE::Math::TVector<double>&, ECollisionChannel, const FCollisionQueryParams&, const TArray<TEnumAsByte<EObjectTypeQuery>>&, FName, ETraceType, int, UObject*);

/**
 * 
 */
UCLASS()
class TRACERPRO_API UTracerProMisc : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
		
		static bool LineTraceMulti(TArray<FHitResult>& OutHits, const FVector& VStart, const FVector& End,ECollisionChannel TraceChannel,const FCollisionQueryParams& Params,
		const TArray<TEnumAsByte<EObjectTypeQuery>> & ObjectTypes, FName ProfileName, ETraceType TraceType, int32 MaxCount, UObject* WorldContextObject)
	{
		OutHits.Reset();
		UWorld* World = WorldContextObject->GetWorld();
		switch (TraceType)
		{
		case ETraceType::TrChannel: World->LineTraceMultiByChannel(OutHits, VStart, End, TraceChannel, Params); break;
		case ETraceType::TrObject:  World->LineTraceMultiByObjectType(OutHits, VStart, End, ObjectTypes, Params); break;
		case ETraceType::TrProfile: World->LineTraceMultiByProfile(OutHits, VStart, End, ProfileName, Params); break;
		}
		if(OutHits.Num())
		{
			if(MaxCount > 0)
				if(OutHits.Num() > MaxCount)
					OutHits.RemoveAt(MaxCount, OutHits.Num()-MaxCount);
			
			return true;
		}
		return false;
	}
	
	static bool LineTraceSingle(TArray<FHitResult>& OutHits, const FVector& VStart, const FVector& End,ECollisionChannel TraceChannel,const FCollisionQueryParams& Params,
		const TArray<TEnumAsByte<EObjectTypeQuery>> & ObjectTypes, FName ProfileName, ETraceType TraceType, int32 MaxCount, UObject* WorldContextObject)
	{
		UWorld* World = WorldContextObject->GetWorld();
		
		FHitResult H;
		OutHits.Init(H, 1);
		switch (TraceType)
		{
		case ETraceType::TrChannel: return  World->LineTraceSingleByChannel(OutHits[0], VStart, End, TraceChannel, Params);
		case ETraceType::TrObject:  return  World->LineTraceSingleByObjectType(OutHits[0], VStart, End, ObjectTypes, Params);
		case ETraceType::TrProfile: return  World->LineTraceSingleByProfile(OutHits[0], VStart, End, ProfileName, Params);
		default: return false;
		}
	}
	
	static bool LineTraceBounce(TArray<FHitResult>& OutHits, const FVector& VStart, const FVector& End,ECollisionChannel TraceChannel,const FCollisionQueryParams& Params,
		const TArray<TEnumAsByte<EObjectTypeQuery>> & ObjectTypes, FName ProfileName, ETraceType TraceType, int32 MaxCount, UObject* WorldContextObject)
	{
		UWorld* World = WorldContextObject->GetWorld();
		OutHits.Reset();
		FVector S = VStart;
		FVector E = End;
		int32 Count = MaxCount;
		do
		{
			TArray<FHitResult> H;

			if(LineTraceSingle(H, S, E, TraceChannel, Params, ObjectTypes, ProfileName, TraceType, MaxCount, WorldContextObject))
			{
				OutHits.Emplace(H[0]);
				Count--;
				FVector B = (H[0].TraceEnd-H[0].TraceStart).MirrorByVector(H[0].ImpactNormal);
				S = B.GetUnsafeNormal() + H[0].ImpactPoint;
				E = B + H[0].ImpactPoint;
			} 
			else return !OutHits.IsEmpty();
		}
		while(!(Count < 0));

		return true;
	}
	
	const inline static TArray<TrType> TraceWork =  {&LineTraceSingle, &LineTraceMulti, &LineTraceBounce};

	static EParallelForFlags ParallelTest()
	{
		if(FPlatformProcess::SupportsMultithreading())
		{
			if(IsInGameThread()) return EParallelForFlags::None;
			return EParallelForFlags::BackgroundPriority;
		}
		return EParallelForFlags::ForceSingleThread;
	}

	static FVector VectorRotate(const FVector R, const FQuat& AddRotation)
	{
		const FVector Q = FVector{AddRotation.X, AddRotation.Y, AddRotation.Z};

		return (Q*(Q.Dot(R)*2)) + ((FMath::Pow(AddRotation.W, 2) - Q.Dot(Q)) * R) + (Q.Cross(R) * AddRotation.W * 2);
	}

	static FVector GetRotated(FVector A, FRotator Angle)
	{
		return Angle.IsZero() ? A : VectorRotate(A, Angle.Quaternion());
	}
	
	
	static TArray<FHitResult> RunTrace(TArray<TArray<FVector>> Base, TrType CurrentTrace, bool Visualize, ECollisionChannel CollisionChannel, const FCollisionQueryParams& CollisionParams,
		const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, FName ProfileName, ETraceType TraceType, int MaxCount, UObject* WorldContextObject)
	{
		UWorld* W = WorldContextObject->GetWorld();
		TArray<FHitResult> TraceResults;
		FCriticalSection Mutex;
		
		const int32 MStart = Base[0].Num() > 1 ? 1 : 0;
		const int32 MFinal = Base[1].Num() > 1 ? 1 : 0;
		
		ParallelFor(FMath::Max(Base[0].Num(), Base[1].Num()), [&](int32 j)
		{
			TArray<FHitResult> Hits;
			if(CurrentTrace(Hits, Base[0][j*MStart],Base[1][j*MFinal], UEngineTypes::ConvertToCollisionChannel(ETraceTypeQuery(CollisionChannel)), CollisionParams, ObjectTypes, ProfileName, TraceType, MaxCount, WorldContextObject))
			{
				Mutex.Lock();
				TraceResults+=Hits;
				Mutex.Unlock();
		
		#if WITH_EDITOR
		if(Visualize)
			AsyncTask(ENamedThreads::GameThread, [Hits, j, MStart, Base, W]()
			{
				for (auto H : Hits)
				{
					DrawDebugLine(W, H.TraceStart, H.Location, FColor::Red, false, 5);
					DrawDebugBox (W, H.Location, FVector(2,2,2),H.ImpactNormal.Rotation().Quaternion(),FColor::Green,false,5);
				}
		});
		#endif
				}
		#if WITH_EDITOR
		else
		{
			AsyncTask(ENamedThreads::GameThread, [j, MStart, MFinal, Base, W, Visualize]()
			{
			if(Visualize) DrawDebugLine(W, Base[0][j*MStart],Base[1][j*MFinal], FColor::Green, false, 5);
			});
		}
		#endif
				}, ParallelTest());

		return TraceResults;
	}

	UFUNCTION(BlueprintCallable, Category="TracerPro")
	static TArray<FTransform> GetHitsTransforms(TArray<FHitResult> OutHits)
	{
		TArray<FTransform> Result;
		
		FCriticalSection Mutex;

		const int32 Threads =  FPlatformMisc::NumberOfCoresIncludingHyperthreads() + 1;
		const int32 HitsPerCore = OutHits.Num() / Threads;
		
		ParallelFor(Threads, [&](const int32 ThreadNum)
		{
			const int32 Start = HitsPerCore * ThreadNum;
			const int32 End = Start + HitsPerCore + (ThreadNum == Threads - 1 ? OutHits.Num() % Threads : 0);
			
			TArray<FTransform> Buffer;
			
			for(int32 T = Start; T<End; ++T)
				Buffer.Emplace(OutHits[T].ImpactNormal.Rotation(),OutHits[T].ImpactPoint);
			
			Mutex.Lock();
				Result += Buffer;
			Mutex.Unlock();
			
		}, ParallelTest());
		return Result;
	}
	
	UFUNCTION(BlueprintCallable, Category="TracerPro")
	static TArray<FHitsActor> GetHitsCount(TArray<FHitResult> OutHits)
	{
		TArray<FHitsActor> Result;
		TArray<AActor*> Actors;
		for (auto H : OutHits)
		{
			Actors.AddUnique(H.GetActor());
			int32 Num = Actors.Find(H.GetActor());
			if(Num >= Result.Num())
			{
				Result.Emplace();
				Result[Num].Actor = Actors[Num];
			}
			Result[Num].HitsCount++;
		}
		return Result;
	}
};
