// SteeringComponent.cpp
#include "SteeringComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

USteeringComponent::USteeringComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 기본값 설정
	bAvoidanceEnabled = true;
	bSeparationEnabled = true;
	SeekWeight = 1.0f;
	SeparationWeight = 1.0f;
	AvoidanceWeight = 1.2f;
	MaxSpeed = 600.0f;
	MaxSeekForce = 1.0f;
	MaxSeparationForce = 1.0f;
	MaxAvoidanceForce = 1.0f;
	SeparationRadius = 300.0f;
	AvoidanceRadius = 500.0f;
	AvoidancePredictionTime = 2.0f;
	ArrivalRadius = 300.0f;
	TargetReachedThreshold = 50.0f;
	bDebugDraw = false;
}

void USteeringComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 소유 캐릭터와 무브먼트 컴포넌트 참조 얻기
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		CharacterMovement = OwnerCharacter->GetCharacterMovement();
	}
	
	// CharacterMovementComponent 초기화
	if (CharacterMovement)
	{
		CharacterMovement->bOrientRotationToMovement = true;
		CharacterMovement->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
		CharacterMovement->MaxWalkSpeed = MaxSpeed;
		CharacterMovement->bUseControllerDesiredRotation = false;
	}
}

void USteeringComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter || !CharacterMovement)
	{
		return;
	}

	// 모든 스티어링 동작의 힘 계산
	FVector SeekForce = CalculateSeekForce() * SeekWeight;
	FVector SeparationForce = FVector::ZeroVector;
	FVector AvoidanceForce = FVector::ZeroVector;
	
	if (bSeparationEnabled)
	{
		SeparationForce = CalculateSeparationForce() * SeparationWeight * MaxSpeed;
	}
	
	if (bAvoidanceEnabled)
	{
		AvoidanceForce = CalculateAvoidanceForce() * AvoidanceWeight * MaxSpeed;
	}

	// 모든 힘 합치기
	FVector SteeringForce = SeekForce + SeparationForce + AvoidanceForce;
	SteeringForce.Z = 0.0f; // Z축 힘은 제거 (2D 평면에서 이동)

	// 최종 방향벡터로 변환 (정규화)
	FVector SteeringDirection = SteeringForce.GetSafeNormal();

	// 캐릭터에 입력 벡터 추가
	OwnerCharacter->AddMovementInput(SteeringDirection);

	// MaxWalkSpeed 조정 (속도 제한)
	CharacterMovement->MaxWalkSpeed = MaxSpeed;

	// 디버그 시각화
	if (bDebugDraw)
	{
		const FVector Start = OwnerCharacter->GetActorLocation();
		DrawDebugLine(GetWorld(), Start, Start + SeekForce, FColor::Green, false, -1.0f, 0, 3.0f);
		DrawDebugLine(GetWorld(), Start, Start + SeparationForce, FColor::Blue, false, -1.0f, 0, 3.0f);
		DrawDebugLine(GetWorld(), Start, Start + AvoidanceForce, FColor::Red, false, -1.0f, 0, 3.0f);
		DrawDebugLine(GetWorld(), Start, Start + SteeringDirection, FColor::Yellow, false, -1.0f, 0, 5.0f);
	}
}

void USteeringComponent::SetTargetLocation(const FVector& NewLocation)
{
	TargetLocation = NewLocation;
}

void USteeringComponent::SetAvoidanceEnabled(bool bEnabled)
{
	bAvoidanceEnabled = bEnabled;
}

FVector USteeringComponent::CalculateSeekForce()
{
	if (!OwnerCharacter)
	{
		return FVector::ZeroVector;
	}

	const FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	const FVector Direction = TargetLocation - CurrentLocation;
	const float Distance = Direction.Size();

	// 목표에 도달했는지 확인
	if (Distance < TargetReachedThreshold)
	{
		return FVector::ZeroVector;
	}

	// 기본 방향 벡터
	FVector DesiredVelocity = Direction.GetSafeNormal() * MaxSpeed;

	// 목표 지점 근처에서 감속
	if (Distance < ArrivalRadius)
	{
		DesiredVelocity *= (Distance / ArrivalRadius);
	}

	// 현재 속도와 원하는 속도의 차이가 스티어링 힘
	FVector CurrentVelocity = OwnerCharacter->GetVelocity();
	// FVector SeekForce = DesiredVelocity - CurrentVelocity;
	FVector SeekForce = DesiredVelocity;
	
	// if (SeekForce.IsNearlyZero())
	// {
	// 	SeekForce = DesiredVelocity;
	// }

	// 최대 힘으로 클램핑
	// if (SeekForce.SizeSquared() > (MaxSeekForce * MaxSeekForce))
	// {
	// 	SeekForce = SeekForce.GetSafeNormal() * MaxSeekForce;
	// }

	return SeekForce;
}

FVector USteeringComponent::CalculateSeparationForce()
{
	if (!OwnerCharacter)
	{
		return FVector::ZeroVector;
	}

	TArray<ACharacter*> NearbyCharacters = FindNearbyCharacters();
	if (NearbyCharacters.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	FVector SeparationForce = FVector::ZeroVector;
	const FVector CurrentLocation = OwnerCharacter->GetActorLocation();

	// 주변 캐릭터로부터 멀어지는 힘 계산
	for (ACharacter* OtherCharacter : NearbyCharacters)
	{
		if (!OtherCharacter || OtherCharacter == OwnerCharacter)
		{
			continue;
		}

		FVector ToOther = CurrentLocation - OtherCharacter->GetActorLocation();
		float Distance = ToOther.Size();

		// 분리 반경 내에 있는 캐릭터만 처리
		if (Distance < SeparationRadius && Distance > 0.0f)
		{
			// 거리에 반비례하는 힘 계산 (더 가까울수록 더 강한 힘)
			FVector RepulsionDir = ToOther.GetSafeNormal();
			float RepulsionStrength = 1.0f - (Distance / SeparationRadius); // 선형 감소
			SeparationForce += RepulsionDir * RepulsionStrength;
		}
	}

	// 주변 캐릭터가 있으면 힘 정규화
	if (!SeparationForce.IsNearlyZero())
	{
		// 최대 힘으로 클램핑
		if (SeparationForce.SizeSquared() > (MaxSeparationForce * MaxSeparationForce))
		{
			SeparationForce = SeparationForce.GetSafeNormal() * MaxSeparationForce;
		}
	}

	return SeparationForce;
}

FVector USteeringComponent::CalculateAvoidanceForce()
{
	if (!OwnerCharacter)
	{
		return FVector::ZeroVector;
	}

	TArray<ACharacter*> NearbyCharacters = FindNearbyCharacters();
	if (NearbyCharacters.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	FVector AvoidanceForce = FVector::ZeroVector;
	const FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	const FVector CurrentVelocity = OwnerCharacter->GetVelocity();

	// 미래 충돌을 예측하여 회피
	for (ACharacter* OtherCharacter : NearbyCharacters)
	{
		if (!OtherCharacter || OtherCharacter == OwnerCharacter)
		{
			continue;
		}

		// 둘 다 움직이고 있는지 확인
		const FVector OtherVelocity = OtherCharacter->GetVelocity();
		if (CurrentVelocity.IsNearlyZero() && OtherVelocity.IsNearlyZero())
		{
			continue;
		}

		const FVector OtherLocation = OtherCharacter->GetActorLocation();
		
		// 상대 위치와 상대 속도 계산
		const FVector RelativePosition = OtherLocation - CurrentLocation;
		const FVector RelativeVelocity = OtherVelocity - CurrentVelocity;
		
		// 현재 거리 확인
		const float CurrentDistance = RelativePosition.Size();
		
		// 회피 반경을 벗어난 캐릭터는 무시
		if (CurrentDistance > AvoidanceRadius)
		{
			continue;
		}

		// 충돌 시간 계산 (상대 속도 방향으로 투영)
		const float RelativeSpeed = RelativeVelocity.Size();
		if (RelativeSpeed < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		// 충돌 예측 시간 계산
		const float TimeToCollision = FVector::DotProduct(RelativePosition, RelativeVelocity) / (RelativeSpeed * RelativeSpeed);
		
		// 과거 충돌이나 너무 먼 미래 충돌은 무시
		if (TimeToCollision <= 0.0f || TimeToCollision > AvoidancePredictionTime)
		{
			continue;
		}

		// 예측 위치에서의 거리 계산
		const FVector FutureRelativePosition = RelativePosition + RelativeVelocity * TimeToCollision;
		const float FutureDistance = FutureRelativePosition.Size();
		
		// 캐릭터 크기를 고려한 충돌 임계값 (두 캐릭터의 캡슐 반경 합)
		const float CollisionThreshold = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() + 
		                                 OtherCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
        
		// 미래에 충돌할 것으로 예측되면 회피력 계산
		if (FutureDistance < CollisionThreshold * 2.0f) // 약간의 여유 공간 추가
		{
			// 충돌 위치에서 멀어지는 방향
			FVector AvoidDir = FutureRelativePosition.GetSafeNormal();
			
			// 충돌 가능성에 기반한 회피력 (가까울수록, 빨리 충돌할수록 강한 힘)
			float AvoidStrength = (1.0f - (TimeToCollision / AvoidancePredictionTime)) * 
			                       (1.0f - (FutureDistance / (CollisionThreshold * 2.0f)));
			
			// 현재 이동 방향에 수직인 회피 벡터 생성 (방향 보존을 위해)
			FVector CurrentDir = CurrentVelocity.GetSafeNormal();
			if (!CurrentDir.IsNearlyZero())
			{
				// 회피 방향과 현재 방향의 내적이 양수면 오른쪽, 음수면 왼쪽으로 회피
				float DotProduct = FVector::DotProduct(CurrentDir, AvoidDir);
				FVector AvoidanceDir = FVector::CrossProduct(CurrentDir, FVector::UpVector);
				
				// 더 적절한 회피 방향 선택
				if (DotProduct < 0.0f)
				{
					AvoidanceDir = -AvoidanceDir;
				}
				
				AvoidanceForce += AvoidanceDir * AvoidStrength;
			}
			else
			{
				// 정지 상태일 경우 그냥 반대 방향으로 피함
				AvoidanceForce -= AvoidDir * AvoidStrength;
			}
		}
	}

	// 회피력 최대치 제한
	if (!AvoidanceForce.IsNearlyZero())
	{
		if (AvoidanceForce.SizeSquared() > (MaxAvoidanceForce * MaxAvoidanceForce))
		{
			AvoidanceForce = AvoidanceForce.GetSafeNormal() * MaxAvoidanceForce;
		}
	}

	return AvoidanceForce;
}

TArray<ACharacter*> USteeringComponent::FindNearbyCharacters()
{
	TArray<ACharacter*> Result;
	
	if (!OwnerCharacter || !GetWorld())
	{
		return Result;
	}

	// 전체 캐릭터 목록 얻기 - 실제 게임에서는 공간 분할 구조 사용 권장
	TArray<AActor*> AllCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllCharacters);

	const FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	const float MaxRadius = FMath::Max(SeparationRadius, AvoidanceRadius);

	// 반경 내 캐릭터 필터링
	for (AActor* Actor : AllCharacters)
	{
		ACharacter* OtherCharacter = Cast<ACharacter>(Actor);
		if (!OtherCharacter || OtherCharacter == OwnerCharacter)
		{
			continue;
		}

		// 거리 확인
		const float Distance = FVector::Dist(CurrentLocation, OtherCharacter->GetActorLocation());
		if (Distance < MaxRadius)
		{
			Result.Add(OtherCharacter);
		}
	}

	return Result;
}