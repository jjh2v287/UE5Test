// Copyright 2024 BitProtectStudio. All Rights Reserved.


#include "WDActorComponent.h"

#include "NiagaraComponent.h"

#include "TimerManager.h"
#include "DirectorActor.h"

#include "Kismet/GameplayStatics.h"
#include "Engine.h"


// Sets default values for this component's properties
UWDActorComponent::UWDActorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UWDActorComponent::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> allActorsArr_;
	UGameplayStatics::GetAllActorsOfClass(this, ADirectorActor::StaticClass(), allActorsArr_);
	if (allActorsArr_.IsValidIndex(0))
	{
		if (allActorsArr_[0])
		{
			directorActorRef = Cast<ADirectorActor>(allActorsArr_[0]);
		}
	}

	hiddenTickComponentsInterval = FMath::RandRange(0.1f, 0.25f);

	ownerActor = Cast<AActor>(GetOwner());

	TArray<UActorComponent*> allTickComponents_;
	GetOwner()->GetComponents(allTickComponents_);
	for (int compID_ = 0; compID_ < allTickComponents_.Num(); ++compID_)
	{
		defaultTickComponentsInterval.Add(allTickComponents_[compID_]->GetComponentTickInterval());
	}
	defaultTickActorInterval = ownerActor->GetActorTickInterval();
	hiddenTickActorInterval = hiddenTickComponentsInterval;

	float rateOptimization_ = FMath::RandRange(0.1f, 0.2f);
	
	// Optimization timer.
	GetWorld()->GetTimerManager().SetTimer(actorOptimization_Timer, this, &UWDActorComponent::OptimizationTimer, rateOptimization_, true, rateOptimization_);

	float randRate_ = FMath::RandRange(0.1f, 1.f);
	// Register Actor timer.
	GetWorld()->GetTimerManager().SetTimer(registerActor_Timer, this, &UWDActorComponent::RegisterActorTimer, randRate_, true, randRate_);
}

void UWDActorComponent::RegisterActorTimer()
{
	if (!bIsRegistered && bIsActivate && directorActorRef)
	{
		if (GetOwner())
		{
			if (ownerActor != nullptr)
			{
				if (!Cast<APawn>(ownerActor))
				{
					if (directorActorRef->RegisterActor(Cast<AActor>(ownerActor)))
					{
						bIsRegistered = true;
						GetWorld()->GetTimerManager().ClearTimer(registerActor_Timer);
					}
				}
				else
				{
					GetWorld()->GetTimerManager().ClearTimer(registerActor_Timer);
				}
			}
		}
	}
}

void UWDActorComponent::OptimizationTimer()
{
	if (!Cast<APawn>(ownerActor) && bIsActivate)
	{
		TArray<UActorComponent*> allComponents = GetOwner()->GetComponentsByTag(UPrimitiveComponent::StaticClass(), componentsTag);

		bool bIsCameraSeeActor_ = IsCameraSeeActor();

		for (int i = 0; i < allComponents.Num(); ++i)
		{
			UPrimitiveComponent* primitiveComponent_ = Cast<UPrimitiveComponent>(allComponents[i]);
			if (primitiveComponent_)
			{
				primitiveComponent_->SetVisibility(bIsCameraSeeActor_);
			}

			USkeletalMeshComponent* skeletalMesh_ = Cast<USkeletalMeshComponent>(allComponents[i]);
			if (skeletalMesh_)
			{
				if (bIsCameraSeeActor_)
				{
					skeletalMesh_->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
				}
				else
				{
					skeletalMesh_->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
				}
			}
		}

		// Set Tick Interval for all components in actor.
		TArray<UActorComponent*> allTickComponents_;
		GetOwner()->GetComponents(allTickComponents_);

		if (allTickComponents_.Num() > 0 && bIsOptimizeAllActorComponentsTickInterval)
		{
			if (bIsCameraSeeActor_)
			{
				for (int compID_ = 0; compID_ < allTickComponents_.Num(); ++compID_)
				{
					if (bIsDisableTickIfBehindCameraFOV)
					{
						if (!Cast<UNiagaraComponent>(allTickComponents_[compID_]))
						{
							allTickComponents_[compID_]->SetComponentTickEnabled(true);
						}
						ownerActor->SetActorTickEnabled(true);
					}
					else if (defaultTickComponentsInterval.IsValidIndex(compID_))
					{
						allTickComponents_[compID_]->SetComponentTickInterval(defaultTickComponentsInterval[compID_]);
						ownerActor->SetActorTickInterval(defaultTickActorInterval);
					}
				}
			}
			else
			{
				for (int compID_ = 0; compID_ < allTickComponents_.Num(); ++compID_)
				{
					if (bIsDisableTickIfBehindCameraFOV)
					{
						if (!Cast<UNiagaraComponent>(allTickComponents_[compID_]))
						{
							allTickComponents_[compID_]->SetComponentTickEnabled(false);
						}
						ownerActor->SetActorTickEnabled(false);
					}
					else
					{
						allTickComponents_[compID_]->SetComponentTickInterval(hiddenTickComponentsInterval);
						ownerActor->SetActorTickInterval(hiddenTickActorInterval);
					}
				}
			}
		}

		if (bIsCameraSeeActor_)
		{
			BroadcastInCameraFOV();
		}
		else
		{
			BroadcastBehindCameraFOV();
		}
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(actorOptimization_Timer);
	}
}

bool UWDActorComponent::IsCameraSeeActor() const
{
	bool bIsSee_ = true;

	APlayerCameraManager* playerCam_ = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (playerCam_)
	{
		AActor* playerActor_ = Cast<AActor>(playerCam_->GetOwningPlayerController());
		if (playerActor_)
		{
			if (playerCam_->GetDotProductTo(GetOwner()) < 1.f - playerCam_->GetFOVAngle() / 100.f &&
				(playerCam_->GetCameraLocation() - GetOwner()->GetActorLocation()).Size() > distanceCamara)
			{
				bIsSee_ = false;
			}
			else
			{
				bIsSee_ = true;
			}
		}
	}

	return bIsSee_;
}


// Called every frame
void UWDActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UWDActorComponent::BroadcastOnPrepareForOptimization() const
{
	OnPrepareForOptimization.Broadcast();
}

void UWDActorComponent::BroadcastOnRecoveryFromOptimization() const
{
	OnRecoveryFromOptimization.Broadcast();
}

void UWDActorComponent::BroadcastBehindCameraFOV() const
{
	EventBehindCameraFOV.Broadcast();
}

void UWDActorComponent::BroadcastInCameraFOV() const
{
	EventInCameraFOV.Broadcast();
}

void UWDActorComponent::SetActorUniqName(FString setName)
{
	actorUniqName = setName;
}

FString UWDActorComponent::GetActorUniqName()
{
	return actorUniqName;
}
