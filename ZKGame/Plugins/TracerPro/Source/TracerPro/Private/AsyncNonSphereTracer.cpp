// Copyright 2024, Cubeap Electronics, All rights reserved

#include "AsyncNonSphereTracer.h"

UAsyncNonSphereTracer* UAsyncNonSphereTracer::AsyncNonSphereTrace(const UObject* WorldContextObject, EFigureType FigureType, bool TraceComplex, FVector Start, float Width, float Height, float Length,
                                                                  FRotator Angle, EMethod TraceMethod, int32 Value, EModificator Modificator, int32 MaxCount, TArray<AActor*> ActorsToIgnore, ETraceProType TraceType, ETraceTypeQuery CollisionChannel,
                                                                  TArray<TEnumAsByte<EObjectTypeQuery>>  ObjectTypes, FName ProfileName, bool Visualize)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	UAsyncNonSphereTracer* Node = NewObject<UAsyncNonSphereTracer>();
	
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->CollisionParams.bTraceComplex = TraceComplex;
	Node->CollisionParams.AddIgnoredActors(ActorsToIgnore); 
	Node->Start = Start;
	FVector2D D = FVector2D{Width, Height};
	Node->Diameters = {D * ((int32)FigureType & 1), D};
	Node->Modificator = Modificator;
	Node->MaxCount = MaxCount < 0 ? 0 : MaxCount;
	Node->ObjectTypes = ObjectTypes;
	Node->ProfileName = ProfileName;
	Node->Step = Value < 1 ? 1 : Value;
	Node->Angle = Angle;
	Node->Length = Length;
	Node->Visualize = Visualize;

#if UE_VERSION_OLDER_THAN(4, 27, 99)
	switch (Modificator)
	{
	case EModificator::TrBounce:
		Node->CurrentTrace = &UTracerProMisc::LineTraceBounce;
		break;
	case EModificator::TrMultiLine:
		Node->CurrentTrace = &UTracerProMisc::LineTraceMulti;
		break;
	default:Node->CurrentTrace = &UTracerProMisc::LineTraceSingle;
	}
#endif
	
#if UE_VERSION_NEWER_THAN(4, 27, 99)
	Node->CurrentTrace = UTracerProMisc::TraceWork[(int32)Modificator];
#endif
	
	Node->TraceType = TraceType;
	Node->TraceMethod = TraceMethod;
	
	if((int32)FigureType & 2)
	{
		Node->Testing = &UAsyncNonSphereTracer::CircleTest;
		Node->Rand = &UAsyncNonSphereTracer::CircleRandom;
	}
	if((int32)TraceMethod) Node->Method = &UAsyncNonSphereTracer::FixedCount;
	
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());

	Node->CollisionChannel = CollisionChannel;
	
	return Node;
}


TArray<TArray<FVector>> UAsyncNonSphereTracer::FixedStep()
{
	TArray<TArray<FVector>> Base;
	Base.Init(TArray<FVector>(),2);

	const TArray<FVector> StartPoints = {FVector::ZeroVector, FVector{Length,0,0}};
	
	ParallelFor(2, [&](int32 i)
	{
		Base[i] = StepGenerate(Diameters[i],StartPoints[i]);
	}, UTracerProMisc::ParallelTest());

	return Base;
}

TArray<FVector> UAsyncNonSphereTracer::StepGenerate(const FVector2D& D, const FVector& Add)
{
	TArray<FVector> V;
	FVector2D S = D/2;
	int32 Z = 0;
	do
	{
		int32 Y = 0;
		do
		{
			FVector A = FVector{0,Y-S.X,Z-S.Y} + Add;
			if((this->*(Testing))(FVector2D{float(Y),float(Z)} - S, D))
				V.Emplace(UTracerProMisc::GetRotated(A, Angle) + Start);
			Y+=Step;
		} while (Y <= D.X);
		Z+=Step;
	} while (Z <= D.Y);
	return V;
}

TArray<TArray<FVector>> UAsyncNonSphereTracer::FixedCount()
{
	TArray<TArray<FVector>> Base;
	Base.Init(TArray<FVector>(),2);
	
	const TArray<FVector> StartPoints = {FVector::ZeroVector, FVector{Length,0,0}};
	
	ParallelFor(Diameters.Num(), [&](int32 j)
	{
		if(!Diameters[j].IsZero())
		{
			Base[j].Init(FVector{0}, Step);
			ParallelFor(Step, [&](int32 i)
			{
				Base[j][i] = UTracerProMisc::GetRotated((this->*(Rand))() + StartPoints[j], Angle) + Start;
			}, UTracerProMisc::ParallelTest());
		}
		else Base[j].Init(Start, 1);
	}, UTracerProMisc::ParallelTest());

	return Base;
}

void UAsyncNonSphereTracer::Activate()
{
	AsyncTask(ENamedThreads::BackgroundThreadPriority, [this]
	{
		TraceResults = UTracerProMisc::RunTrace((this->*(Method))(), CurrentTrace, Visualize, ECollisionChannel(CollisionChannel), CollisionParams, ObjectTypes, ProfileName, TraceType, MaxCount, WorldContextObject);
		
		AsyncTask(ENamedThreads::GameThread, [this]()
			{
				Completed.Broadcast(TraceResults);
			});
	 });
}