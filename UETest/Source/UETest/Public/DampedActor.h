// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DampedActor.generated.h"

UCLASS()
class UETEST_API ADampedActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADampedActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	// 액터의 헤더 파일에 선언
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ForceVector; // 적용할 힘 벡터
	
	FVector OriginalPosition; // 객체의 원래 위치를 저장
	FVector NewPosition;
	float ElapsedTime = 0.0f; // 경과 시간
	bool bIsShaking = false; // 흔들림 상태 여부
	FVector CalculateDampedOscillation(float DeltaTime, FVector StartPosition, FVector Force, float Duration);

	FVector DirectionNormal = FVector(0.0f, -1.0f, 0.0f);
	// 초기 위치
	FVector InitialPosition;
	// 초기 진폭
	float InitialAmplitude = 50.0f;
	// 감쇠율
	float Damping = 10.0f;
	// 각속도
	float AngularVelocity = 60.0f;

	float MaxTime = 0.2f;
	// 시간 변수
	float TimeElapsed = 0.0f;
	// 시간 변수
	float StartTime = 0.0f;
};
