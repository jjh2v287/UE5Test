// Fill out your copyright notice in the Description page of Project Settings.


#include "DampedActor.h"

// Sets default values
ADampedActor::ADampedActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADampedActor::BeginPlay()
{
	Super::BeginPlay();
	// 초기 위치 설정
	InitialPosition = GetActorLocation();
}

// Called every frame
void ADampedActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// NewPosition = CalculateDampedOscillation(DeltaTime, OriginalPosition, ForceVector, 2.0f); // 2초 동안 진동
	// SetActorLocation(NewPosition);

	StartTime +=DeltaTime;
	if(StartTime < 1.0f)
	{
		return;
	}
	
	{
		// 움직임의 진폭을 결정하는 변수를 도입
		TimeElapsed += DeltaTime;
		float HalfMaxTime = (MaxTime * 0.5f);
		float Amplitude;
	    
		// 1초 동안은 진폭을 일정하게 유지
		if (TimeElapsed <= HalfMaxTime)
		{
			Amplitude = InitialAmplitude;
		}
		// 1초 후에는 진폭을 시간에 따라 감소시켜 원점으로 돌아오게 함
		else
		{
			float ReturnTime = (TimeElapsed - HalfMaxTime);
			Amplitude = InitialAmplitude * FMath::Exp(-Damping * ReturnTime);
		}

		// 현재 위치를 업데이트
		FVector NewPosition2 = InitialPosition + (DirectionNormal * Amplitude * FMath::Sin(AngularVelocity * TimeElapsed));
		SetActorLocation(NewPosition2);

		if(TimeElapsed >= MaxTime*5)
		{
			TimeElapsed = 0.0f;
			SetActorLocation(InitialPosition);
		}
	}
}

// 감쇠 진동을 계산하는 함수
FVector ADampedActor::CalculateDampedOscillation(float DeltaTime, FVector StartPosition, FVector Force, float Duration)
{
	ElapsedTime += DeltaTime;

	// 탄성 계수와 감쇠 비율
	float Elasticity = 0.1f; // 탄성 계수, 낮을수록 더 많이 흔들림
	float Damping2 = 10.0f; // 감쇠 비율, 높을수록 빠르게 진정됨

	FVector Displacement = Force * Elasticity * cos(sqrt(Elasticity) * ElapsedTime) * exp(-Damping2 * ElapsedTime);
    
	if (ElapsedTime >= Duration) {
		bIsShaking = false;
		ElapsedTime = 0.0f; // 시간 초기화
		return StartPosition; // 원래 위치로 복귀
	}

	return StartPosition + Displacement; // 새로운 위치 계산
}
