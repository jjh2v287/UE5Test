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
	void GravityTick(float DeltaTime);
	void SimpleTick(float DeltaTime);
	
	bool CheckWalkable(const FVector& FloorNormal) const;
	bool FindGround(const FVector& QueryOrigin, FHitResult& OutHit, const float DeltaTime) const;
	
protected:
	// 중력 방향 단위 벡터 (일반적으로 FVector::DownVector (0,0,-1))
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gravity")
	// FVector GravityDirection = FVector::DownVector;

	// 현재 중력에 의해 누적된 속도
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Movement|Debug")
	FVector GravityVelocity = FVector::ZeroVector;
    
	// 낙하 최대 속도 (Terminal Velocity)
	UPROPERTY(EditAnywhere, Category = "Movement|Gravity")
	float MaxFallSpeed = 4000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move")
	float MaxSpeed = 70.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Ground")
	float MaxStepUp = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Ground")
	float MaxStepDown = 60.f;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Movement|Debug")
	FVector CurrentFloorNormal = FVector::UpVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Ground")
	float GroundSnapDistance = 80.f; // 아래로 찾을 거리
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Ground")
	float WalkableFloorAngleDeg = 45.f;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Movement|Debug")
	USkeletalMeshComponent* Mesh;
};
