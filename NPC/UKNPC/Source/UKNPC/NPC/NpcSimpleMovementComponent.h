// NpcSimpleMovementComponent.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "NpcSimpleMovementComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNpcSimpleMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move")
	float MaxSpeed = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Ground")
	float GroundSnapDistance = 80.f; // 아래로 찾을 거리

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Ground")
	float MaxStepUp = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Ground")
	float MaxStepDown = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Ground")
	float WalkableFloorAngleDeg = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Gravity")
	float GravityZ = -980.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Gravity")
	float TerminalSpeed = 4000.f;
	
	void BeginPlay() override;
	
	// AIController(PathFollowingComponent)가 이동 요청을 보낼 때 호출되는 함수
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;

	// 외부에서 원하는 이동 입력(월드 방향)을 넣어주면 됨 (AI/BT/PathFollowing 등)
	void SetDesiredVelocityXY(const FVector& InVelXY);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	FVector DesiredVelXY = FVector::ZeroVector;
	float VerticalSpeed = 0.f;
	bool bGrounded = true;

	bool FindGround(const FVector& QueryOrigin, FHitResult& OutHit) const;
	bool CheckWalkable(const FVector& FloorNormal) const;

	FVector ProjectOnPlane(const FVector& V, const FVector& N) const
	{
		return V - FVector::DotProduct(V, N) * N;
	}
};
