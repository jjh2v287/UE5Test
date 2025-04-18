﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "UKECSAgentComponent.h"
#include "UKECSComponentBase.h"
#include "UKECSManager.h"
#include "UKAgent.h"
#include "Engine/World.h"

UUKECSAgentComponent::UUKECSAgentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUKECSAgentComponent::InitData()
{
	if (!RunECS)
	{
		return;
	}
	
	// 1. WorldSubsystem에서 ECS 매니저 가져오기
	UUKECSManager* ECSManager = GetWorld()->GetSubsystem<UUKECSManager>();
	if (!ECSManager)
	{
		return;
	}
    
	// 2. 아키타입 구성 정의 (Position + Velocity)
	FECSArchetypeComposition MovableComposition;
	for (const FInstancedStruct& Struct : ECSComponentInstances)
	{
		if (Struct.IsValid())
		{
			MovableComposition.Add(Struct.GetScriptStruct());
		}
	}

	// 3. 엔티티 생성
	Entity = ECSManager->CreateEntity(MovableComposition);
	
	// 4. 컴포넌트 데이터 설정
	if (FUKECSForwardMoveComponent* MoveComponent = ECSManager->GetComponentData<FUKECSForwardMoveComponent>(Entity))
	{
		MoveComponent->Location = GetOwner()->GetActorLocation();
		MoveComponent->ForwardVector = GetOwner()->GetActorForwardVector();
		MoveComponent->Rotator = GetOwner()->GetActorRotation();
		MoveComponent->TargetLocation = MoveComponent->Location + (MoveComponent->ForwardVector * 100000.0f);
		MoveComponent->Speed = 100.0f;
		MoveComponent->OwnerActor = GetOwner();
	}

	if (FUKECSAvoidMoveComponent* MoveComponent = ECSManager->GetComponentData<FUKECSAvoidMoveComponent>(Entity))
	{
		if (AUKAgent* Agent = Cast<AUKAgent>(GetOwner()))
		{
			MoveComponent->OwnerActor = Agent;
			MoveComponent->MaxSpeed = Agent->MaxSpeed;
			MoveComponent->MaxAcceleration = Agent->MaxAcceleration;
			MoveComponent->AgentRadius = Agent->AgentRadius;
			MoveComponent->ObstacleDetectionDistance = Agent->ObstacleDetectionDistance;
			MoveComponent->SeparationRadiusScale = Agent->SeparationRadiusScale;
			MoveComponent->ObstacleSeparationDistance = Agent->ObstacleSeparationDistance;
			MoveComponent->ObstacleSeparationStiffness = Agent->ObstacleSeparationStiffness;
			MoveComponent->PredictiveAvoidanceTime = Agent->PredictiveAvoidanceTime;
			MoveComponent->PredictiveAvoidanceRadiusScale = Agent->PredictiveAvoidanceRadiusScale;
			MoveComponent->PredictiveAvoidanceDistance = Agent->PredictiveAvoidanceDistance;
			MoveComponent->ObstaclePredictiveAvoidanceStiffness =  Agent->ObstaclePredictiveAvoidanceStiffness;
			MoveComponent->StandingObstacleAvoidanceScale = Agent->StandingObstacleAvoidanceScale;
			MoveComponent->OrientationSmoothingTime = Agent->OrientationSmoothingTime;
			MoveComponent->AgentHandle = Agent->GetAgentHandle();
		
			MoveComponent->MoveTargetLocation = GetOwner()->GetActorLocation() + (GetOwner()->GetActorForwardVector() * 10000.0f);
			MoveComponent->MoveTargetForward = GetOwner()->GetActorForwardVector();
			MoveComponent->DesiredSpeed = MoveComponent->MaxSpeed;
			MoveComponent->DistanceToGoal = 0.0f;
		}
	}

	// 5. 시스템 로직 등록/실행
	for (TSubclassOf<UUKECSSystemBase>& System : ECSSystemBaseClasses)
	{
		ECSManager->AddSystem(System);
	}
}

// Called when the game starts
void UUKECSAgentComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UUKECSAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/*if (!RunECS)
	{
		return;
	}
	
	UUKECSManager* ECSManager = GetWorld()->GetSubsystem<UUKECSManager>();
	if (!ECSManager)
	{
		return;
	}
    
	if (FUKECSForwardMoveComponent* MoveComponent = ECSManager->GetComponentData<FUKECSForwardMoveComponent>(Entity))
	{
		const FVector Location = MoveComponent->Location;
		const FRotator Rotator = MoveComponent->Rotator;
		GetOwner()->SetActorLocation(Location);
		GetOwner()->SetActorRotation(Rotator);
	}*/
}

