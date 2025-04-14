// Fill out your copyright notice in the Description page of Project Settings.


#include "UKECSSystemBase.h"

#include "DrawDebugHelpers.h"
#include "UKAgent.h"
#include "UKECSComponentBase.h"
#include "UKECSManager.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

/*---------- Move Calculation ----------*/
void UECSMove::Register()
{
	QueryTypes.Emplace(FUKECSForwardMoveComponent::StaticStruct());
}

void UECSMove::Tick(float DeltaTime, UUKECSManager* ECSManager)
{
	ECSManager->ForEachChunk(QueryTypes, [&](FECSArchetypeChunk& Chunk)
	{
		int32 NumInChunk = Chunk.GetEntityCount();
        
		// 청크에서 직접 데이터 배열 가져오기 (SoA 방식)
		FUKECSForwardMoveComponent* MoveComponent = static_cast<FUKECSForwardMoveComponent*>(Chunk.GetComponentRawDataArray(FUKECSForwardMoveComponent::StaticStruct()));

		if (MoveComponent)
		{
			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [=]()
			{
				ParallelFor(NumInChunk, [&](int32 i)
				{
					FVector Direction = FVector(MoveComponent[i].TargetLocation - MoveComponent[i].Location);
					MoveComponent[i].ForwardVector = Direction.GetSafeNormal();
					MoveComponent[i].Location += MoveComponent[i].ForwardVector * MoveComponent[i].Speed * DeltaTime;
					MoveComponent[i].Rotator = MoveComponent[i].ForwardVector.Rotation();
				});
			});
			
			/*for (int32 i = 0; i < NumInChunk; ++i)
			{
				FVector Direction = FVector(MoveComponent[i].TargetLocation - MoveComponent[i].Location);
				MoveComponent[i].ForwardVector = Direction.GetSafeNormal();
				MoveComponent[i].Location += MoveComponent[i].ForwardVector * MoveComponent[i].Speed * DeltaTime;
				MoveComponent[i].Rotator = MoveComponent[i].ForwardVector.Rotation();
			}*/
		}
	});
}

/*---------- Move Synchronization ----------*/
void UECSMoveSync::Register()
{
	QueryTypes.Emplace(FUKECSForwardMoveComponent::StaticStruct());
}

void UECSMoveSync::Tick(float DeltaTime, UUKECSManager* ECSManager)
{
	ECSManager->ForEachChunk(QueryTypes, [&](FECSArchetypeChunk& Chunk)
	{
		int32 NumInChunk = Chunk.GetEntityCount();
        
		// 청크에서 직접 데이터 배열 가져오기 (SoA 방식)
		FUKECSForwardMoveComponent* MoveComponent = static_cast<FUKECSForwardMoveComponent*>(Chunk.GetComponentRawDataArray(FUKECSForwardMoveComponent::StaticStruct()));

		if (MoveComponent)
		{
			AsyncTask(ENamedThreads::GameThread, [=]()
			{
				// FCriticalSection Mutex;
				// Mutex.Lock();
				// UObject 작업 수행
				for (int32 i = 0; i < NumInChunk; ++i)
				{
					if (MoveComponent[i].OwnerActor.IsValid())
					{
						MoveComponent[i].OwnerActor.Get()->SetActorLocation(MoveComponent[i].Location);
						MoveComponent[i].OwnerActor.Get()->SetActorRotation(MoveComponent[i].Rotator);
					}
				}
				// Mutex.Unlock();
			});
			
			/*for (int32 i = 0; i < NumInChunk; ++i)
			{
				if (MoveComponent[i].OwnerActor.IsValid())
				{
					MoveComponent[i].OwnerActor.Get()->SetActorLocation(MoveComponent[i].Location);
					MoveComponent[i].OwnerActor.Get()->SetActorRotation(MoveComponent[i].Rotator);
				}
			}*/
		}
	});
}

/*---------- AvoidMove Synchronization ----------*/
void UECSAvoidMove::Register()
{
	QueryTypes.Emplace(FUKECSAvoidMoveComponent::StaticStruct());
	NavigationManage = GetWorld()->GetSubsystem<UUKAgentManager>();
}

void UECSAvoidMove::Tick(float DeltaTime, UUKECSManager* ECSManager)
{
	ECSManager->ForEachChunk(QueryTypes, [&](FECSArchetypeChunk& Chunk)
	{
		int32 NumInChunk = Chunk.GetEntityCount();
		// 청크에서 직접 데이터 배열 가져오기 (SoA 방식)
		FUKECSAvoidMoveComponent* MoveComponent = static_cast<FUKECSAvoidMoveComponent*>(Chunk.GetComponentRawDataArray(FUKECSAvoidMoveComponent::StaticStruct()));
		if (!MoveComponent)
		{
			return;
		}
		
		// AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [=]()
		// {
			ParallelFor(NumInChunk, [&](int32 i)
			{
				TArray<AUKAgent*> NearbyActors;
				if (NavigationManage)
				{
					NearbyActors = NavigationManage->FindCloseAgentRange(MoveComponent[i].OwnerActor.Get(), MoveComponent[i].ObstacleDetectionDistance);
					// FindNearbyActors(NearbyActors, ObstacleDetectionDistance);
				}

				// 회피력 계산
				MoveComponent[i].SeparationForce = CalculateSeparationForce(NearbyActors, MoveComponent[i]);
				MoveComponent[i].PredictiveAvoidanceForce = CalculatePredictiveAvoidanceForce(NearbyActors, MoveComponent[i]);
				FVector EnvironmentForce = CalculateEnvironmentAvoidanceForce();
				MoveComponent[i].SteerForce = CalculateSteeringForce(DeltaTime, MoveComponent[i]);
			    
				// 총 이동력 계산
				MoveComponent[i].SteeringForce = MoveComponent[i].SteerForce + MoveComponent[i].SeparationForce + MoveComponent[i].PredictiveAvoidanceForce + EnvironmentForce;
			    
				// 최대 가속도로 제한
				if (MoveComponent[i].SteeringForce.SizeSquared() > FMath::Square(MoveComponent[i].MaxAcceleration))
				{
					MoveComponent[i].SteeringForce = MoveComponent[i].SteeringForce.GetSafeNormal() * MoveComponent[i].MaxAcceleration;
				}
			    
				// 속도 업데이트
				MoveComponent[i].CurrentVelocity += MoveComponent[i].SteeringForce * DeltaTime;
			    
				// 최대 속도로 제한
				if (MoveComponent[i].CurrentVelocity.SizeSquared() > FMath::Square(MoveComponent[i].MaxSpeed))
				{
					MoveComponent[i].CurrentVelocity = MoveComponent[i].CurrentVelocity.GetSafeNormal() * MoveComponent[i].MaxSpeed;
				}
				// Actor 속도 엡데이트
				MoveComponent[i].OwnerActor->CurrentVelocity = MoveComponent[i].CurrentVelocity;
			    
				// 위치 업데이트
				MoveComponent[i].NewLocation = MoveComponent[i].OwnerActor.Get()->GetActorLocation() + MoveComponent[i].CurrentVelocity * DeltaTime;
			    
				// 방향 업데이트 (부드러운 회전)
				if (MoveComponent[i].CurrentVelocity.SizeSquared() > 1.0f)
				{
					FVector DesiredForward = MoveComponent[i].CurrentVelocity.GetSafeNormal();
					FVector CurrentForward = MoveComponent[i].OwnerActor.Get()->GetActorForwardVector();
			        
					// 현재 Yaw 각도
					float CurrentYaw = FMath::Atan2(CurrentForward.Y, CurrentForward.X);
			        
					// 목표 Yaw 각도
					float DesiredYaw = FMath::Atan2(DesiredForward.Y, DesiredForward.X);
			        
					// 각도 차이 계산 (Wrap 처리)
					float DeltaAngle = DesiredYaw - CurrentYaw;
					if (DeltaAngle > PI) DeltaAngle -= PI * 2;
					if (DeltaAngle < -PI) DeltaAngle += PI * 2;
			        
					// 부드러운 각도 변화
					float SmoothingRate = DeltaTime / FMath::Max(MoveComponent[i].OrientationSmoothingTime, 0.01f);
					float NewYaw = CurrentYaw + DeltaAngle * SmoothingRate;
			        
					// 회전 적용
					MoveComponent[i].NewRotation = FRotator(0, FMath::RadiansToDegrees(NewYaw), 0);
				}
			});
		// });
	});
}

FVector UECSAvoidMove::CalculateSeparationForce(const TArray<AUKAgent*>& NearbyActors, FUKECSAvoidMoveComponent& AvoidMoveComponent)
{
	FVector TotalForce = FVector::ZeroVector;
    
	// 분리 반경 계산
	float SeparationRadius = AvoidMoveComponent.AgentRadius * AvoidMoveComponent.SeparationRadiusScale;
    
	for (AUKAgent* OtherActor : NearbyActors)
	{
		// 상대 위치 계산
		FVector RelPos = AvoidMoveComponent.OwnerActor.Get()->GetActorLocation() - OtherActor->GetActorLocation();
		RelPos.Z = 0; // 평면 위에서만 고려
        
		float Distance = RelPos.Size();
		FVector Direction = RelPos / FMath::Max(Distance, 0.01f); // 0으로 나누기 방지
        
		// 다른 액터의 반경 고려
		float OtherRadius = OtherActor->AgentRadius;
		float TotalRadius = SeparationRadius + OtherRadius;
        
		// 분리 페널티 계산
		float SeparationPenalty = (TotalRadius + AvoidMoveComponent.ObstacleSeparationDistance) - Distance;
        
		// 정지 상태 액터는 회피력 감소
		float StandingScaling = 1.0f;
		if (OtherActor->CurrentVelocity.SizeSquared() < 1.0f)
		{
			StandingScaling = AvoidMoveComponent.StandingObstacleAvoidanceScale;
		}
        
		// 거리에 따른 분리력 감소 (Falloff)
		float SeparationMag = FMath::Square(FMath::Clamp(SeparationPenalty / AvoidMoveComponent.ObstacleSeparationDistance, 0.0f, 1.0f));
        
		// 분리력 계산
		FVector SeparationForce = Direction * AvoidMoveComponent.ObstacleSeparationStiffness * SeparationMag * StandingScaling;
        
		// 총 분리력에 추가
		TotalForce += SeparationForce;
	}
    
	return TotalForce;
}


FVector UECSAvoidMove::CalculatePredictiveAvoidanceForce(const TArray<AUKAgent*>& NearbyActors, FUKECSAvoidMoveComponent& AvoidMoveComponent)
{
	FVector TotalForce = FVector::ZeroVector;
    
	// 예측적 회피 반경 계산
	float AvoidanceRadius = AvoidMoveComponent.AgentRadius * AvoidMoveComponent.PredictiveAvoidanceRadiusScale;
    
	for (AUKAgent* OtherActor : NearbyActors)
	{
		// 상대 위치와 속도 계산
		FVector RelPos = AvoidMoveComponent.OwnerActor.Get()->GetActorLocation() - OtherActor->GetActorLocation();
		RelPos.Z = 0; // 평면 위에서만 고려
		FVector RelVel = AvoidMoveComponent.CurrentVelocity - OtherActor->CurrentVelocity;
        
		// 다른 액터의 반경 고려
		float OtherRadius = OtherActor->AgentRadius;
		float TotalRadius = AvoidanceRadius + OtherRadius;
        
		// 정지 상태 액터는 회피력 감소
		float StandingScaling = 1.0f;
		if (OtherActor->CurrentVelocity.SizeSquared() < 1.0f)
		{
			StandingScaling = AvoidMoveComponent.StandingObstacleAvoidanceScale;
		}
        
		// 가장 가까운 접근 시간(CPA) 계산
		float CPA = ComputeClosestPointOfApproach(RelPos, RelVel, TotalRadius, AvoidMoveComponent.PredictiveAvoidanceTime);
        
		// CPA에서의 상대 위치 계산
		FVector AvoidRelPos = RelPos + RelVel * CPA;
		float AvoidDist = AvoidRelPos.Size();
		FVector AvoidNormal = AvoidDist > 0.01f ? (AvoidRelPos / AvoidDist) : FVector::ForwardVector;
        
		// 예측 침투 계산
		float AvoidPenetration = (TotalRadius + AvoidMoveComponent.PredictiveAvoidanceDistance) - AvoidDist;
        
		// 침투에 따른 회피력 계산
		float AvoidMag = FMath::Square(FMath::Clamp(AvoidPenetration / AvoidMoveComponent.PredictiveAvoidanceDistance, 0.0f, 1.0f));
        
		// 거리에 따른 감소
		float AvoidMagDist = (1.0f - (CPA / AvoidMoveComponent.PredictiveAvoidanceTime));
        
		// 최종 회피력 계산
		FVector AvoidForce = AvoidNormal * AvoidMag * AvoidMagDist * AvoidMoveComponent.ObstaclePredictiveAvoidanceStiffness * StandingScaling;
        
		// 총 회피력에 추가
		TotalForce += AvoidForce;
	}
    
	return TotalForce;
}

FVector UECSAvoidMove::CalculateEnvironmentAvoidanceForce()
{
	// 환경 회피력은 별도의 충돌/레이캐스트 로직으로 구현해야 함
	// 여기서는 기본적인 틀만 제공
	FVector TotalForce = FVector::ZeroVector;
    
	// TODO: 레이캐스트나 오버랩 테스트를 통해 환경 장애물 감지 및 회피력 계산
    
	return TotalForce;
}

FVector UECSAvoidMove::CalculateSteeringForce(float DeltaTime, FUKECSAvoidMoveComponent& AvoidMoveComponent)
{
	// 목표까지 방향 및 거리 계산
	FVector DirectionToTarget = AvoidMoveComponent.MoveTargetLocation - AvoidMoveComponent.OwnerActor.Get()->GetActorLocation();
	DirectionToTarget.Z = 0; // 평면 위에서만 고려
    
	AvoidMoveComponent.DistanceToGoal = DirectionToTarget.Size();
    
	// 목표 지점에 도달한 경우
	if (AvoidMoveComponent.DistanceToGoal < 1.0f)
	{
		return -AvoidMoveComponent.CurrentVelocity * (1.0f / FMath::Max(DeltaTime, 0.1f)); // 정지력
	}
    
	// 정규화된 방향
	FVector Direction = DirectionToTarget / AvoidMoveComponent.DistanceToGoal;
    
	// 현재 전방 벡터
	FVector CurrentForward = AvoidMoveComponent.OwnerActor.Get()->GetActorForwardVector();
    
	// 방향에 따른 속도 스케일링 (전방 이동이 가장 빠름)
	float ForwardDot = FVector::DotProduct(CurrentForward, Direction);
	float SpeedScale = 0.5f + 0.5f * FMath::Max(0.0f, ForwardDot); // 0.5 ~ 1.0 범위
    
	// 목표 속도 계산
	AvoidMoveComponent.DesiredVelocity = Direction * AvoidMoveComponent.DesiredSpeed * SpeedScale;
    
	// 도착 시 감속
	float ArrivalDistance = 200.0f; // 감속 시작 거리
	if (AvoidMoveComponent.DistanceToGoal < ArrivalDistance)
	{
		float SpeedFactor = FMath::Sqrt(AvoidMoveComponent.DistanceToGoal / ArrivalDistance); // 감속 커브
		AvoidMoveComponent.DesiredVelocity *= SpeedFactor;
	}
    
	// 조향력 계산 (목표 속도와 현재 속도의 차이)
	float SteerStrength = 1.0f / 0.3f; // 반응 시간의 역수
	FVector SteerForce = (AvoidMoveComponent.DesiredVelocity - AvoidMoveComponent.CurrentVelocity) * SteerStrength;
    
	return SteerForce;
}


float UECSAvoidMove::ComputeClosestPointOfApproach(const FVector& RelPos, const FVector& RelVel, float TotalRadius, float TimeHorizon)
{
	// 상대 속도와 위치로 충돌 시간 계산
	float A = FVector::DotProduct(RelVel, RelVel);
	float Inv2A = A > KINDA_SMALL_NUMBER ? 1.0f / (2.0f * A) : 0.0f;
	float B = FMath::Min(0.0f, 2.0f * FVector::DotProduct(RelVel, RelPos));
	float C = FVector::DotProduct(RelPos, RelPos) - FMath::Square(TotalRadius);
    
	// 판별식 계산 (음수인 경우 최대값으로)
	float Discr = FMath::Sqrt(FMath::Max(0.0f, B * B - 4.0f * A * C));
    
	// 최단 충돌 시간 계산
	float T = (-B - Discr) * Inv2A;
    
	// 시간 범위 제한
	return FMath::Clamp(T, 0.0f, TimeHorizon);
}

void UECSAvoidMoveSync::Register()
{
	Super::Register();
	QueryTypes.Emplace(FUKECSAvoidMoveComponent::StaticStruct());
	NavigationManage = GetWorld()->GetSubsystem<UUKAgentManager>();
}

void UECSAvoidMoveSync::Tick(float DeltaTime, UUKECSManager* ECSManager)
{
	Super::Tick(DeltaTime, ECSManager);
	ECSManager->ForEachChunk(QueryTypes, [&](FECSArchetypeChunk& Chunk)
	{
		int32 NumInChunk = Chunk.GetEntityCount();
        
		// 청크에서 직접 데이터 배열 가져오기 (SoA 방식)
		FUKECSAvoidMoveComponent* MoveComponent = static_cast<FUKECSAvoidMoveComponent*>(Chunk.GetComponentRawDataArray(FUKECSAvoidMoveComponent::StaticStruct()));

		if (MoveComponent)
		{
			for (int32 i = 0; i < NumInChunk; ++i)
			{
				if (MoveComponent[i].OwnerActor.IsValid())
				{
					MoveComponent[i].OwnerActor.Get()->SetActorRotation(MoveComponent[i].NewRotation);
					MoveComponent[i].OwnerActor.Get()->SetActorLocation(MoveComponent[i].NewLocation);
					if (NavigationManage)
					{
						NavigationManage->AgentMoveUpdate(MoveComponent[i].OwnerActor.Get());
					}

					/*FVector StartLocation = MoveComponent[i].OwnerActor.Get()->GetActorLocation() + FVector(0,0, 150);
					DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + MoveComponent[i].CurrentVelocity, 15, FColor::Black, false, -1, 0, 3.0f);
					StartLocation += FVector(0,0, 2);
					DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + MoveComponent[i].SeparationForce, 15, FColor::Yellow, false, -1, 0, 3.0f);
					StartLocation += FVector(0,0, 4);
					DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + MoveComponent[i].PredictiveAvoidanceForce, 15, FColor::Purple, false, -1, 0, 3.0f);
					StartLocation += FVector(0,0, 6);
					DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + MoveComponent[i].SteerForce, 15, FColor::Red, false, -1, 0, 3.0f);
					StartLocation += FVector(0,0, 8);
					DrawDebugDirectionalArrow(GetWorld(), StartLocation, StartLocation + MoveComponent[i].SteeringForce, 15, FColor::Green, false, -1, 0, 3.0f);

					// DrawDebugSphere(GetWorld(), GetActorLocation(), AgentRadius, 12, FColor::Green, false, -1, 0, 1.0f);
					DrawDebugSphere(GetWorld(), MoveComponent[i].MoveTargetLocation, MoveComponent[i].AgentRadius, 12, FColor::Red, false, -1, 0, 1.0f);*/
				}
			}
		}
	});
}

