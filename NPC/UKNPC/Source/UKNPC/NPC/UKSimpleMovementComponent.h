// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "UKSimpleMovementComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UKNPC_API UUKSimpleMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UUKSimpleMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// AIController(PathFollowingComponent)가 이동 요청을 보낼 때 호출되는 함수
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;
	
protected:
	void SimpleWalkingUpdate(float DeltaTime);
	
	bool CheckWalkable(const FVector& FloorNormal) const;
	bool FindGround(const FVector& QueryOrigin, FHitResult& OutHit, const float DeltaTime) const;
	
protected:
	// 낙하 최대 속도 (Terminal Velocity)
	UPROPERTY(EditAnywhere, Category = "Move|Simple")
	float MaxFallSpeed = 4000.0f;
	
	// 아래로 찾을 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Simple")
	float GroundSnapDistance = 10.f;
	
	// 현재 중력에 의해 누적된 속도
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Move|Simple")
	FVector GravityVelocity = FVector::ZeroVector;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Move|Simple")
	USkeletalMeshComponent* Mesh;
};
