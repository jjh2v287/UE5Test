// SteeringComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SteeringComponent.generated.h"

// 전방 선언
class ACharacter;
class UCharacterMovementComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ZKGAME_API USteeringComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USteeringComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 목표 위치 설정
	UFUNCTION(BlueprintCallable, Category = "AI|Steering")
	void SetTargetLocation(const FVector& NewLocation);

	// 회피 동작 활성화/비활성화
	UFUNCTION(BlueprintCallable, Category = "AI|Steering")
	void SetAvoidanceEnabled(bool bEnabled);

private:
	// 목표 위치로 이동하는 스티어링 벡터
	FVector CalculateSeekForce();

	// 다른 NPC와의 분리 스티어링 벡터
	FVector CalculateSeparationForce();

	// 다른 NPC와의 회피 스티어링 벡터
	FVector CalculateAvoidanceForce();

	// 주변의 다른 NPC 찾기
	TArray<ACharacter*> FindNearbyCharacters();

private:
	// 소유 캐릭터 참조
	UPROPERTY()
	ACharacter* OwnerCharacter;

	// 캐릭터 무브먼트 참조
	UPROPERTY()
	UCharacterMovementComponent* CharacterMovement;

	// 목표 위치
	UPROPERTY(VisibleAnywhere, Category = "AI|Steering")
	FVector TargetLocation;

	// 회피 활성화 여부
	UPROPERTY(EditAnywhere, Category = "AI|Steering")
	bool bAvoidanceEnabled;

	// 분리 활성화 여부
	UPROPERTY(EditAnywhere, Category = "AI|Steering")
	bool bSeparationEnabled;

	// Seek 행동 가중치
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float SeekWeight;

	// 분리 행동 가중치
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float SeparationWeight;

	// 회피 행동 가중치
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float AvoidanceWeight;

	// 최대 속력
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float MaxSpeed;

	// 최대 Seek 힘
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float MaxSeekForce;

	// 최대 분리 힘
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float MaxSeparationForce;

	// 최대 회피 힘
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float MaxAvoidanceForce;

	// 분리 감지 반경
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float SeparationRadius;

	// 회피 감지 반경
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float AvoidanceRadius;
	
	// 회피 예측 시간 (초)
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float AvoidancePredictionTime;

	// 도착 감속 반경
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float ArrivalRadius;
	
	// 목표에 도달했다고 간주할 거리
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float TargetReachedThreshold;

	// 디버그 시각화 활성화 여부
	UPROPERTY(EditAnywhere, Category = "AI|Steering|Debug")
	bool bDebugDraw;
};