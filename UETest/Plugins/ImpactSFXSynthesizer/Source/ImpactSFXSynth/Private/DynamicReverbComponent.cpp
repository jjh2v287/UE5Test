// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "DynamicReverbComponent.h"

#include "HRTFModal.h"
#include "ImpactSFXSynthLog.h"
#include "ModalReverb.h"
#include "Engine/World.h"
#include "Containers/Array.h"
#include "TimerManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/HitResult.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DynamicReverbComponent)

UDynamicReverbComponent::UDynamicReverbComponent()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bEnableHRTF = true;
	HeadRadius = 0.15f;
	
	TraceTick = 1.0f / 30.f; //30 frames per second
	TraceChannel = ECC_Camera;
	NumTracePerFrame = 8;
	NumDoneTraces = 0;
	NumInterFrame = 8;
	ResetDistance = 1000.0f;
	CurrentTraceYawnRange = 0.f;
	CurrentTracePitchRange = 70.f;
	TraceEndVector = FVector(6000.f, 0.f, 0.f);	
	HeightOffset = 0.f;
	
	CurrentRoomSize = 100.f;
	CurrentAbsorption = 2.f;
	CurrentOpenRoomFactor = 10.f;
	CurrentLocation = FVector::Zero();
	LastLocation = FVector::Zero();
	bIsRoomStatUpdated = true;
	
	CollisionQueryParams = FCollisionQueryParams(TEXT("DynamicReverbTrace"), false, GetOwner());
	CollisionResponseParams = FCollisionResponseParams::DefaultResponseParam;
	Collisions.Empty(NumTracePerFrame);
	
	bIsDrawTraceDebug = false;
	DebugDrawTime = 1.f;
}

void UDynamicReverbComponent::PostInitProperties()
{
	Super::PostInitProperties();

	ResetTempBuffers();
}

void UDynamicReverbComponent::ResetTempBuffers()
{
	RoomSizeArray.Empty(NumInterFrame);
	EchoVarArray.Empty(NumInterFrame);
	NumNoHitArray.Empty(NumInterFrame);
	InterFrameIndex = 0;
	NumTotalMiss = 0;
}

void UDynamicReverbComponent::GetRoomStats(const FVector& AudioLocation, float& OutRoomSize, float& OutOpenFactor,
	float& OutAbsorption, float& OutEchoVar) const
{
	const float Distance = FVector::Distance(AudioLocation, CurrentLocation);
	const float InThreshold = CurrentRoomSize + CurrentEchoVar; 
	if(Distance <= InThreshold)
	{
		OutRoomSize = CurrentRoomSize;
		OutOpenFactor = CurrentOpenRoomFactor;
	}
	else
	{
		const float Scale = Distance / InThreshold;
		const float ScaleSqrt = Scale * Scale;
		OutRoomSize = FMath::Min(LBSImpactSFXSynth::FModalReverb::MaxRoomSize, CurrentRoomSize * ScaleSqrt);
		OutOpenFactor = CurrentOpenRoomFactor * ScaleSqrt;
	}
	
	OutAbsorption = CurrentAbsorption;
	OutEchoVar = CurrentEchoVar;
}

void UDynamicReverbComponent::SetHeadRadius(const float InHeadRadius)
{
	HeadRadius = InHeadRadius;
	LBSImpactSFXSynth::FHRTFModal::SetGlobalHeadRadius(InHeadRadius);
}

void UDynamicReverbComponent::BeginPlay()
{
	Super::BeginPlay();
	
	LBSImpactSFXSynth::FHRTFModal::SetGlobalHeadRadius(HeadRadius);
	
	if(AActor* Owner = GetOwner())
	{
		LastLocation = Owner->GetActorLocation();
		LastLocation.Z += HeightOffset;
	}
	
	NumDoneTraces = 0;
	TraceDelegate.BindUObject(this, &UDynamicReverbComponent::OnTraceCompleted);
	GetWorld()->GetTimerManager().SetTimer(TraceTimerHandle, this, &UDynamicReverbComponent::OnStartTrace, TraceTick, true);
}

void UDynamicReverbComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void UDynamicReverbComponent::OnStartTrace()
{
	if(!bIsRoomStatUpdated)
	{
		UE_LOG(LogImpactSFXSynth, Log, TEXT("UDynamicReverbComponent::OnStartTrace: Skipped due to unfinished trace frame."));
		return;
	}
	
	UWorld* World = GetWorld();
	if(World == nullptr)
		return;
	
	AActor* Owner = GetOwner();
	if(!IsValid(Owner))
		return;
	
	CurrentLocation = Owner->GetActorLocation();
	CurrentLocation.Z += HeightOffset;
	
	NumDoneTraces = 0;
	bIsRoomStatUpdated = false;
	Collisions.Empty(NumTracePerFrame);

	const FQuat OwnerRotator = FQuat(Owner->GetActorRotation());
	for(int i = 0; i < NumTracePerFrame; i++)
	{
		if(i % 4 == 0)
		{
			CurrentTraceYawnRange = FMath::FRand() * 90.f;
			CurrentTracePitchRange = -CurrentTracePitchRange;
		}
		
		const float RandPitch = FMath::FRand() * CurrentTracePitchRange + 10.f;
		
		const FQuat Rotator = OwnerRotator * FQuat(FRotator(RandPitch, CurrentTraceYawnRange, 0.f));
		const FVector EndVector = Rotator.RotateVector(TraceEndVector) + CurrentLocation;

		CurrentTraceYawnRange += 90.f;
		
#if WITH_EDITOR
		if(bIsDrawTraceDebug)
		{
			DrawDebugLine(World, CurrentLocation, EndVector,
						FColor(0, 255, 0),false, TraceTick, 0,1);
		}
#endif
		
		World->AsyncLineTraceByChannel(EAsyncTraceType::Single, CurrentLocation, EndVector,
										TraceChannel, CollisionQueryParams, CollisionResponseParams,
										&TraceDelegate);
	}
}

void UDynamicReverbComponent::OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	// Check to make sure this component is still valid
	if(!TraceTimerHandle.IsValid())
		return;
	
	NumDoneTraces++;
	
	if (Data.OutHits.Num() > 0 && Data.OutHits[0].bBlockingHit)
	{
		Collisions.Emplace(Data.OutHits[0].Location);
				
#if WITH_EDITOR
		if(bIsDrawTraceDebug)
		{
			DrawDebugSphere(GetWorld(), Data.OutHits[0].Location, 10, 6,
							FColor(255, 0, 0), false, 
							DebugDrawTime, 0, 0.5f);
		}

#endif
	}
	
	if(!bIsRoomStatUpdated && (NumDoneTraces >= NumTracePerFrame))
	{
		UpdateRoomStats();
		bIsRoomStatUpdated = true;
	}
}

void UDynamicReverbComponent::UpdateRoomStats()
{
	const float DeltaDist = FVector::DistSquared(CurrentLocation, LastLocation);
	LastLocation = CurrentLocation;
	if(DeltaDist > ResetDistance)
		ResetTempBuffers();

	const int32 NumCollision = Collisions.Num();
	//If no collision at first frame, then treat it as in open air
	if(NumCollision == 0 && RoomSizeArray.Num() == 0)
	{
		//No collision, which means no reverb, so just set this absorption to be really large
		CurrentRoomSize = LBSImpactSFXSynth::FModalReverb::MaxRoomSize;
		CurrentAbsorption =  LBSImpactSFXSynth::FModalReverb::NoReverbAbsorption;
		CurrentOpenRoomFactor = LBSImpactSFXSynth::FModalReverb::MaxOpenRoomFactor;
		CurrentEchoVar = 0.f;
		return;
	}
	
	const int32 NumMiss = NumTracePerFrame - NumCollision; 
	if(NumNoHitArray.Num() < NumInterFrame)
	{
		NumNoHitArray.Emplace(NumMiss);
		NumTotalMiss += NumMiss;
	}
	else
	{
		NumTotalMiss -= NumNoHitArray[InterFrameIndex];
		NumNoHitArray[InterFrameIndex] = NumMiss;
		NumTotalMiss += NumMiss;
	}
	
	const float Radius = CalculateRoomStats();
	if(RoomSizeArray.Num() < NumInterFrame)
	{
		RoomSizeArray.Emplace(Radius);
		InterFrameIndex = 0;
		
		if(RoomSizeArray.Num() == 1)
		{
			CurrentRoomSize = Radius;
			EchoVarArray.Emplace(0);
			CurrentEchoVar = 0.f;
		}
		else
		{
			const float NewDelta = Radius - CurrentRoomSize;
			EchoVarArray.Emplace(NewDelta);
			CurrentRoomSize = CurrentRoomSize + NewDelta / RoomSizeArray.Num();
			CurrentEchoVar = CurrentEchoVar + (NewDelta - CurrentEchoVar) / EchoVarArray.Num();
		}
	}
	else
	{
		//Avg rolling calculation: NewAvg = OldAvg + (NewValue - OldValue) / N
		const float OldValue = RoomSizeArray[InterFrameIndex];
		const float DeltaUpdate = Radius - OldValue;
		CurrentRoomSize = CurrentRoomSize + DeltaUpdate / NumInterFrame;
		CurrentEchoVar = CurrentEchoVar + (DeltaUpdate - EchoVarArray[InterFrameIndex]) / NumInterFrame;
		
		RoomSizeArray[InterFrameIndex] = Radius;
		EchoVarArray[InterFrameIndex] = DeltaUpdate;
		InterFrameIndex = (InterFrameIndex + 1) % NumInterFrame;
	}

	constexpr float DecayScale = LBSImpactSFXSynth::FModalReverb::MaxOpenRoomFactor - 1.f;
	CurrentOpenRoomFactor = 1.0f + FMath::Min(DecayScale, (NumTotalMiss * DecayScale) / (NumNoHitArray.Num() * NumTracePerFrame / 2.0f));

	//FUTURE: Detect phys material to change absorption
	CurrentAbsorption = LBSImpactSFXSynth::FModalReverb::DefaultAbsorption;

#if WITH_EDITOR
	if(bLogRoomStats)
		UE_LOG(LogImpactSFXSynth, Log, TEXT("Room Size: %f. Open Room Factor: %f. Echo Var: %f."), CurrentRoomSize, CurrentOpenRoomFactor, CurrentEchoVar);
#endif
}

float UDynamicReverbComponent::CalculateRoomStats()
{
	const int32 NumCollision = Collisions.Num();
	if(NumCollision == 0)
		return LBSImpactSFXSynth::FModalReverb::MaxRoomSize;
	
	float Bottom = 0.f;
	float Top = 0.f;
	float Left = 0.f;
	float Right = 0.f;
	float Back = 0.f;
	float Front = 0.f;
	for(int i = 0; i < NumCollision; i++)
	{
		const FVector Delta = Collisions[i] - CurrentLocation;
			
		if(Bottom > Delta.Z)
			Bottom = Delta.Z;
		else if(Top < Delta.Z)
			Top = Delta.Z;

		if(Back > Delta.X)
			Back = Delta.X;
		else if(Front < Delta.X)
			Front = Delta.X;

		if(Left > Delta.Y)
			Left = Delta.Y;
		else if(Right < Delta.Y)
			Right = Delta.Y;
	}
	
	const float Height = Top - Bottom;
	const float Width = Right - Left;
	const float Length = Front - Back;
	const float Radius = FMath::Pow(Width * Height * Length, 1.f/3.f);
	
	return Radius;
}