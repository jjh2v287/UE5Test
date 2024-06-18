// Copyright 2024, Cubeap Electronics, All rights reserved

#include "AsyncSphereTracer.h"

UAsyncSphereTracer* UAsyncSphereTracer::AsyncSphereTrace(const UObject* WorldContextObject, bool TraceComplex, FVector Start,
                                                         ESphereType SphereType, FRotator Angle, float Radius, ESphereMethod TraceMethod, int32 Count,
                                                         EModificator Modificator, int32 MaxCount, const TArray<AActor*> ActorsToIgnore, ETraceType TraceType,
                                                         ETraceTypeQuery CollisionChannel, const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes, FName ProfileName, bool
                                                         Visualize)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UAsyncSphereTracer* Node = NewObject<UAsyncSphereTracer>();
	
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->CollisionParams.bTraceComplex = TraceComplex;
	Node->CollisionParams.AddIgnoredActors(ActorsToIgnore); 
	Node->Modificator = Modificator;
	Node->MaxCount = MaxCount < 0 ? 0 : MaxCount;
	Node->ObjectTypes = ObjectTypes;
	Node->ProfileName = ProfileName;
	Node->Count = Count < 1 ? 1 : Count;
	Node->Radius = Radius;
	Node->Visualize = Visualize;
	Node->CurrentTrace = UTracerProMisc::TraceWork[(int32)Modificator];
	Node->TraceType = TraceType;
	Node->TraceMethod = TraceMethod;
	Node->Angle = Angle;
	Node->Start = Start;
	if((int32)TraceMethod) Node->GetSphereVector = &UAsyncSphereTracer::RandomVector;
	if((int32)SphereType)
	{
		Node->Default = FRotator{-90,0,0};
		Node->test = (int32)SphereType + 1;
	}
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());

	Node->CollisionChannel = CollisionChannel;
	
	return Node;
}

void UAsyncSphereTracer::Activate()
{
	AsyncTask(ENamedThreads::BackgroundThreadPriority, [this]
	{
		TraceResults = UTracerProMisc::RunTrace(RunSphere(), CurrentTrace, Visualize, ECollisionChannel(CollisionChannel), CollisionParams, ObjectTypes, ProfileName, TraceType, MaxCount, WorldContextObject);
		
		AsyncTask(ENamedThreads::GameThread, [this]()
			{
				Completed.Broadcast(TraceResults);
			});
	 });
}


FVector UAsyncSphereTracer::RandomVector(int32 i, int32 Value) const
{
	return Start +
		UTracerProMisc::GetRotated(
			FVector{Radius,0,0},
			FRotator{FMath::RandRange(Angle.Pitch-180/test,Angle.Pitch+180/test),
				FMath::RandRange(Angle.Yaw-180/test,Angle.Yaw+180/test),
				FMath::RandRange(Angle.Roll-180/test,Angle.Roll+180/test)});
}

FVector UAsyncSphereTracer::FixedVector(int32 i, int32 Value) const
{
	float phi = FMath::Acos(1 - 2 * (i + 0.5) / Value / test);
	float theta = 3.14 * (1 + FMath::Sqrt(5.f)) * i;
	return Start + UTracerProMisc::GetRotated(Radius * FVector{ FMath::Cos(theta) * FMath::Sin(phi), FMath::Sin(theta) * FMath::Sin(phi), FMath::Cos(phi)}, Default + Angle);
}

TArray<TArray<FVector>> UAsyncSphereTracer::RunSphere()
{
	TArray<TArray<FVector>> Base;
	Base.Init(TArray<FVector>(),2);
	Base[0].Emplace(Start);
	
	FCriticalSection Mutex;
	ParallelFor(Count, [&](int32 i)
	{
		FVector A = (this->*(GetSphereVector))(i, Count);
		Mutex.Lock();
			Base[1].Emplace(A);
		Mutex.Unlock();
	}, UTracerProMisc::ParallelTest());
	
	return Base;
}
