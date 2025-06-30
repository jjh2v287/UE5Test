// Fill out your copyright notice in the Description page of Project Settings.


#include "SignificanceComponent.h"

#include "SignificanceManager.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


// Sets default values for this component's properties
USignificanceComponent::USignificanceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void USignificanceComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerActor = GetOwner();
	MovementComponent = OwnerActor->FindComponentByClass<UCharacterMovementComponent>();
	SkeletalMeshComponent = OwnerActor->FindComponentByClass<USkeletalMeshComponent>();
	
	if (const APawn* Pawn = Cast<APawn>(OwnerActor))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			BehaviorTreeComponent = Controller->FindComponentByClass<UBehaviorTreeComponent>();
		}
	}
	
	// Enable URO
	if (SkeletalMeshComponent && bDistanceURO)
	{
		SkeletalMeshComponent->bEnableUpdateRateOptimizations = true;
		
		if (FAnimUpdateRateParameters* AnimUpdateRateParams = SkeletalMeshComponent->AnimUpdateRateParams)
		{
			AnimUpdateRateParams->bShouldUseLodMap = true;
			AnimUpdateRateParams->MaxEvalRateForInterpolation = MaxEvalRateForInterpolation;
			AnimUpdateRateParams->BaseNonRenderedUpdateRate = BaseNonRenderedUpdateRate;
		}
	}
	
	SignificanceCount = SignificanceThresholds.Num();
	SignificanceLastIndex = SignificanceCount - 1; 
	LastLevel = SignificanceThresholds[SignificanceThresholds.Num() - 1].Significance;
	
	auto SignificanceLambda = [this](USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& Viewpoint) -> float
	{
		return SignificanceFunction(ObjectInfo, Viewpoint);
	};

	auto PostSignificanceLambda = [this](USignificanceManager::FManagedObjectInfo* ObjectInfo, const float OldSignificance, const float Significance, const bool bFinal)
	{
		PostSignificanceFunction(ObjectInfo, OldSignificance, Significance, bFinal);
	};

	FName ActorTag = GetOwner()->Tags[0];
	USignificanceManager* SignificanceManager = USignificanceManager::Get(GetWorld());
	SignificanceManager->RegisterObject(GetOwner(), ActorTag, SignificanceLambda, USignificanceManager::EPostSignificanceType::Sequential, PostSignificanceLambda);
}

void USignificanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	USignificanceManager* SignificanceManager = USignificanceManager::Get(GetWorld());

	if (!IsValid(SignificanceManager))
	{
		return;
	}

	SignificanceManager->UnregisterObject(GetOwner());
}

float USignificanceComponent::SignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& Viewpoint)
{
	const FVector Direction = OwnerActor->GetActorLocation() - Viewpoint.GetLocation();
	float Distance = Direction.Size();

	const bool bFrontView = FVector::DotProduct(Viewpoint.GetRotation().GetForwardVector(), Direction) > 0;
	Distance = bFrontView ? Distance : Distance * 10.0f;
	return GetSignificanceByDistance(Distance);
}

float USignificanceComponent::GetSignificanceByDistance(const float Distance)
{
	if (Distance >= SignificanceThresholds[SignificanceLastIndex].MaxDistance)
	{
		return SignificanceThresholds[SignificanceLastIndex].Significance;
	}

	for (int32 i = 0; i < SignificanceCount; ++i)
	{
		if (Distance <= SignificanceThresholds[i].MaxDistance)
		{
			return SignificanceThresholds[i].Significance;
		}
	}
	
	return 0.0f;
}

void USignificanceComponent::PostSignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
{
	if (Significance >= LastLevel)
	{
		SetTickEnabled(false);
	}
	else
	{
		SetTickEnabled(true);

		const float TickInterval = SignificanceThresholds[Significance].TickInterval;

		if (MovementComponent)
		{
			MovementComponent->SetComponentTickInterval(TickInterval);	
		}

		if (SkeletalMeshComponent)
		{
			SkeletalMeshComponent->SetComponentTickInterval(TickInterval);
			if (FAnimUpdateRateParameters* AnimUpdateRateParams = SkeletalMeshComponent->AnimUpdateRateParams)
			{
				int32& FrameSkipCount = AnimUpdateRateParams->LODToFrameSkipMap.FindOrAdd(0);
				FrameSkipCount = SignificanceThresholds[Significance].AnimationFrameSkipCount;
			}
		}
	}

	DrawDebugText(Significance);
}


void USignificanceComponent::SetTickEnabled(const bool bEnabled)
{
	OwnerActor->SetActorTickEnabled(bEnabled);
	
	if (MovementComponent)
	{
		MovementComponent->SetComponentTickEnabled(bEnabled);	
	}

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetComponentTickEnabled(bEnabled);	
	}

	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->SetComponentTickEnabled(bEnabled);	
	}
}

void USignificanceComponent::DrawDebugText(const float Significance) const
{	
	FColor DebugColor = FColor::White;

	if (Significance >= LastLevel)
	{
		DebugColor = FColor::Green;
	}
	else if (Significance >= 1.0f)
	{
		DebugColor = FColor::Yellow;
	}
	
	FVector Location = OwnerActor->GetActorLocation();
	FString ClassName = OwnerActor->GetClass()->GetName();
	FName ActorTag = OwnerActor->Tags[0];
	const FString DebugMsg = FString::Printf(TEXT("Sig[%.1f]\nTag[%s]"), Significance, *ActorTag.ToString());
	Location.Z += 50.0f;
	DrawDebugString(GetWorld(), Location, DebugMsg, nullptr, DebugColor,  1.0f, true);	
}