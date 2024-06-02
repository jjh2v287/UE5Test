// Copyright Kong Studios, Inc. All Rights Reserved.


#include "UKHomingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

UUKHomingComponent::UUKHomingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUKHomingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UUKHomingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	HomingUpdate(DeltaTime);
}

UUKEventTask* UUKHomingComponent::HomingStart(const FUKHomingStartPram HomingStartPram)
{
	AActor* Owner =GetOwner();
	HomingStartInfo = HomingStartPram;
	
    HomingStartRotator = Owner->GetActorRotation();
    HomingTargetActorFindProcess(Owner);

	const bool bNotHomingTarget = !HomingTargetActor.IsValid(); 
	if (bNotHomingTarget)
	{
		HomingStop();
		return nullptr;
	}
	
	HomingState = EUKHomingState::Homing;

	if (HomingEndTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(HomingEndTimer);
	}

	const bool bIsHomingStopTimeer = HomingStartInfo.HomingStopType == EUKHomingStopType::Time;
	if (bIsHomingStopTimeer)
	{
		GetWorld()->GetTimerManager().SetTimer(HomingEndTimer, this, &UUKHomingComponent::HomingStop, HomingStartInfo.Duration);
	}

	TestEventTast = NewObject<UUKEventTask>();
	Owner->GetGameInstance()->RegisterReferencedObject(TestEventTast);
	return TestEventTast;
}

void UUKHomingComponent::HomingStop()
{
	HomingStartRotator = FRotator::ZeroRotator;
	HomingTargetActor = nullptr;
	HomingState = EUKHomingState::Stop;

	if (HomingEndTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(HomingEndTimer);
	}

	if(IsValid(TestEventTast))
	{
		if (TestEventTast.Get()->OnHomingEnd.IsBound())
		{
			TestEventTast.Get()->OnHomingEnd.Broadcast();
		}

		TestEventTast = nullptr;
	}
}

const bool UUKHomingComponent::IsHomingMove() const
{
	return HomingState == EUKHomingState::Homing;
}

const bool UUKHomingComponent::IsHomingStop() const
{
	return HomingState == EUKHomingState::Stop;
}

void UUKHomingComponent::HomingUpdate(const float DeltaTime)
{
	if (IsHomingStop())
	{
		return;
	}
	
	AActor* Owner = GetOwner();
	const float FinalSpeed = HomingStartInfo.HomingRotateSpeed * DeltaTime;
	const FRotator CurrentRotator = Owner->GetActorRotation();
	const FRotator HomingRotator = GetHomingRotator(Owner, HomingTargetActor.Get(), FinalSpeed);
	const float DeltaYaw = GetCalculateAngleDelta(HomingRotator.Yaw, HomingStartRotator.Yaw);
	const bool bOverMaxAngle = FMath::Abs(DeltaYaw) > HomingStartInfo.HomingMaxAngle;
	const bool bIsHomingStopMaxAngle = bOverMaxAngle && HomingStartInfo.HomingStopType == EUKHomingStopType::MaxAngle;
	FRotator FinalRotator = CurrentRotator;
	float FinalAngle = HomingRotator.Yaw;

	if (bOverMaxAngle)
	{
		float MaxAngle = HomingStartInfo.HomingMaxAngle;
		const bool bIsLeft = DeltaYaw < 0.0f;

		if (bIsLeft)
		{
			MaxAngle *= -1.0f;
		}
		
		FinalAngle = HomingStartRotator.Yaw + MaxAngle;
	}

	FinalRotator.Yaw = FinalAngle;
	Owner->SetActorRotation(FinalRotator);
	
	if (bIsHomingStopMaxAngle)
	{
		HomingStop();
	}
}

void UUKHomingComponent::HomingTargetActorFindProcess(AActor* Owner)
{
	const UWorld* World = Owner->GetWorld();
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(World, 0);
	const UBlackboardComponent* Blackboard = UAIBlueprintHelperLibrary::GetBlackboard(Owner);

	const bool IsNotBlackboard = Blackboard == nullptr;
	if (IsNotBlackboard)
	{
		HomingTargetActor = PlayerActor;
		return;
	}

	HomingTargetActor = Cast<AActor>(Blackboard->GetValueAsObject(HomingStartInfo.BlackboardValueName));
	const bool IsNotBlackboardValue = HomingTargetActor == nullptr;
	if (IsNotBlackboardValue)
	{
		HomingTargetActor = PlayerActor;
	}
}

const float UUKHomingComponent::GetCalculateAngleDelta(const float CurrentAngle, const float StartAngle)
{
	float YawDifference = CurrentAngle - StartAngle;

	// Normalize the angle to be within -180 to 180 degrees
	YawDifference = FMath::Fmod(YawDifference + 180.0f, 360.0f);
	if (YawDifference < 0.0f)
	{
		YawDifference += 360.0f;
	}
	YawDifference -= 180.0f;

	return YawDifference;
}

FVector UUKHomingComponent::GetHomingVector(const AActor* Owner, const AActor* Target, const float HomingSpeed)
{
	return GetHomingRotator(Owner, Target, HomingSpeed).Vector();
}

FRotator UUKHomingComponent::GetHomingRotator(const AActor* Owner, const AActor* Target, const float HomingSpeed)
{
	const bool bIsNotCalculation = Owner == nullptr || Target == nullptr;
	if (bIsNotCalculation)
	{
		return FRotator::ZeroRotator;
	}
	
	const FRotator OwnerRotator = Owner->GetActorRotation();
	const FVector DirectionVector = FVector(Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
	const FRotator DirectionRotator = DirectionVector.ToOrientationRotator();

	const FQuat TargetQuat(DirectionRotator);
	const FQuat OwnerQuat(OwnerRotator);
	const FQuat Result = FQuat::Slerp(OwnerQuat, TargetQuat, FMath::Min(HomingSpeed, 1.0f));

	return Result.Rotator();
}