// AISeekCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AISeekCharacter.generated.h"

UCLASS()
class ZKGAME_API AAISeekCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAISeekCharacter();
    
	virtual void Tick(float DeltaTime) override;
    
	// AI를 위한 목표 위치 설정 함수
	UFUNCTION(BlueprintCallable, Category = "AI|Steering")
	void SetTargetLocation(FVector NewTargetLocation);
    
	// 현재 목표 위치 반환
	UFUNCTION(BlueprintPure, Category = "AI|Steering")
	FVector GetTargetLocation() const { return TargetLocation; }
    
	// 목표에 도달했는지 확인
	UFUNCTION(BlueprintPure, Category = "AI|Steering")
	bool HasReachedTarget() const;
    
protected:
	virtual void BeginPlay() override;
    
	// Seek 동작 실행
	void ExecuteSeekBehavior(float DeltaTime);
    
private:
	// 목표 위치
	UPROPERTY(VisibleAnywhere, Category = "AI|Steering")
	FVector TargetLocation;
    
	// 최대 이동 속도 (cm/s)
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float MaxSpeed = 100.0f;
    
	// 최대 스티어링 힘 (방향 변경의 강도)
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float MaxSteeringForce = 1.0f;
    
	// 목표 도착 판정 반경
	UPROPERTY(EditAnywhere, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float ArrivalRadius = 50.0f;
    
	// 이동 중인지 여부
	UPROPERTY(VisibleAnywhere, Category = "AI|Steering")
	bool bIsMoving;
    
	// 목표 위치 디버그 표시 여부
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebug = true;
};