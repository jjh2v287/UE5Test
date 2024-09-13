// Fill out your copyright notice in the Description page of Project Settings.


#include "JHCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AJHCharacter::AJHCharacter()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AJHCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AJHCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AJHCharacter::LaunchToTargetWithFriction(const FVector TargetLocation)
{
	FVector StartLocation = GetActorLocation();
	FVector EndLocation = StartLocation + TargetLocation;
	FVector LaunchVelocity = FVector::ZeroVector;
	float LaunchSpeed = FVector::Distance(StartLocation, EndLocation);  // 초기 속도

	UGameplayStatics::FSuggestProjectileVelocityParameters Params(
		GetWorld(),    // 월드 컨텍스트
		StartLocation, // 시작 위치
		EndLocation,   // 목표 위치
		LaunchSpeed    // 발사 속도
	);
	Params.bDrawDebug = true;
	Params.ActorsToIgnore.Emplace(Cast<AActor>(this));
	
	// bool bSuccess = UGameplayStatics::SuggestProjectileVelocity(Params, LaunchVelocity);
	// if (bSuccess)
	// {
	// 	LaunchCharacter(LaunchVelocity, true, true);
	// }
	// else
	{
		// LaunchVelocity.Z = CalculateLaunchVelocityForHeight(TargetLocation.Z);
		float HangTime = CalculateHangTime(TargetLocation.Z);
		LaunchVelocity.Z = TargetLocation.Z;
		// X와 Y축 이동 계산 (X(t) = X0 + Vx * t, Y(t) = Y0 + Vy * t)
		float Friction = 0.5f * GetPhysicsVolume()->FluidFriction; // 물리 환경의 마찰력
		float BrakingDeceleration = GetCharacterMovement()->GetMaxBrakingDeceleration(); // 제동 감속 값
		float NewVelocity = HangTime - (Friction * HangTime);
		LaunchVelocity.X = TargetLocation.X / (HangTime - 0.005f);
		LaunchVelocity.Y = TargetLocation.Y / (HangTime - 0.005f);
		// LaunchVelocity = CalculateLaunchVelocity2(TargetLocation.Z);
		LaunchCharacter(LaunchVelocity, true, true);
	}
	
	// const float Friction = GetPhysicsVolume()->FluidFriction;
	// const float FrictionCoefficient = GetCharacterMovement()->BrakingDecelerationFalling; // 유체의 마찰 계수 (예: 물)
	// LaunchVelocity = CalculateLaunchVelocityWithFriction(EndLocation, Friction);
	// LaunchCharacter(LaunchVelocity, true, true);
	
	// LaunchVelocity = CalculateLaunchVelocity(StartLocation, EndLocation, 45.0f);
	// LaunchCharacter(LaunchVelocity, true, true);
}

float AJHCharacter::CalculateLaunchVelocityForHeight(float TargetHeight)
{
	float GravityScale = GetCharacterMovement()->GravityScale;
	float WorldGravity = GetWorld()->GetGravityZ();

	// 중력 가속도의 절대값 계산
	float AbsGravityAcceleration = FMath::Abs(WorldGravity * GravityScale);

	// 초기 속도 제곱 계산을 위한 중간 값
	float VelocitySquaredFactor = (TargetHeight * GravityScale) / AbsGravityAcceleration;
	VelocitySquaredFactor *= (GravityScale * 2);

	// 초기 속도 계산
	float InitialVelocity = FMath::Sqrt(VelocitySquaredFactor);

	// 최종 발사 속도 계산
	float LaunchVelocity = (TargetHeight / InitialVelocity) * (GravityScale * 2);

	UE_LOG(LogTemp, Display, TEXT("----------> Launch Velocity Z: %f"), LaunchVelocity);

	return LaunchVelocity;
}

float AJHCharacter::CalculateHangTime(float TargetHeight)
{
	float GravityScale = GetCharacterMovement()->GravityScale;
	float WorldGravity = GetWorld()->GetGravityZ();
	
	// 거리를 안다면 초기 스피가값 구하기
	float HangTime = TargetHeight / FMath::Abs(WorldGravity * GravityScale);
	HangTime = HangTime * 2;
	UE_LOG(LogTemp, Display, TEXT("----------> HangTime %f"), HangTime);
	return HangTime;
}

FVector AJHCharacter::CalculateLaunchVelocity2(float TargetHeight)
{
	// 캐릭터의 현재 Z 위치
	float StartZ = GetActorLocation().Z;
	
	// 목표 높이까지의 실제 거리 계산
	float ActualHeight = TargetHeight - StartZ;
	
	// 실제 중력 계산
	float GravityScale = GetCharacterMovement()->GravityScale;
	float Gravity = FMath::Abs(GetWorld()->GetGravityZ() * GravityScale);
    
	// 공기 저항을 고려한 초기 속도 계산
	const float Friction = GetPhysicsVolume()->FluidFriction;
	const float Braking = GetCharacterMovement()->BrakingDecelerationFalling; // 유체의 마찰 계수 (예: 물)
	float InitialVelocity = FMath::Sqrt(2 * Gravity * TargetHeight / (1 - Friction));
    
	// LaunchVelocity 계산
	FVector LaunchVelocity(0, 0, InitialVelocity);
    
	return LaunchVelocity;
}

FVector AJHCharacter::CalculateLaunchVelocityWithFriction(const FVector& TargetLocation, const float FrictionCoefficient) const
{
	FVector CurrentLocation = GetActorLocation();
	FVector LaunchDirection = TargetLocation - CurrentLocation;
    
	// 수평 거리와 수직 거리 계산
	float HorizontalDistance = FVector::Dist2D(CurrentLocation, TargetLocation);
	float VerticalDistance = LaunchDirection.Z;
    
	// 중력 가져오기
	float Gravity = FMath::Abs(GetWorld()->GetGravityZ());
    
	// 발사 각도 설정 (예: 45도)
	float LaunchAngle = 45.0f * (PI / 180.0f);
    
	// 초기 속도 계산
	float VelocityXY = FMath::Sqrt((Gravity * HorizontalDistance * HorizontalDistance) / (2.0f * (HorizontalDistance * FMath::Tan(LaunchAngle) - VerticalDistance)));
	float VelocityZ = VelocityXY * FMath::Tan(LaunchAngle);
    
	// 마찰을 고려한 속도 보정
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	// float FrictionFactor = 1.0f / (1.0f - FMath::Min(FrictionCoefficient * DeltaTime, 1.0f));
	float FrictionFactor = (1.0f - FMath::Min(FrictionCoefficient * 2, 1.0f));;
	
	VelocityXY *= FrictionCoefficient;
	VelocityZ *= FrictionCoefficient;
    
	// 발사 방향 정규화
	FVector LaunchDirectionNormalized = LaunchDirection.GetSafeNormal2D();
    
	// 최종 발사 속도 계산
	FVector LaunchVelocity = LaunchDirectionNormalized * VelocityXY + FVector::UpVector * VelocityZ;
    
	return LaunchVelocity;
}

FVector AJHCharacter::CalculateLaunchVelocity(FVector Start, FVector Target, float LaunchAngle) const
{
	float Gravity = -GetWorld()->GetGravityZ();
	float Theta = FMath::DegreesToRadians(LaunchAngle);
    
	FVector Diff = Target - Start;
	float DiffXY = FMath::Sqrt(Diff.X * Diff.X + Diff.Y * Diff.Y);
	float DiffZ = Diff.Z;
    
	double V0 = FMath::Sqrt(
		(Gravity * DiffXY * DiffXY) / 
		(2 * FMath::Cos(Theta) * FMath::Cos(Theta) * (DiffXY * FMath::Tan(Theta) - DiffZ))
	);
    
	FVector LaunchVelocity;
	LaunchVelocity.X = V0 * FMath::Cos(Theta) * Diff.X / DiffXY;
	LaunchVelocity.Y = V0 * FMath::Cos(Theta) * Diff.Y / DiffXY;
	LaunchVelocity.Z = V0 * FMath::Sin(Theta);
    
	return LaunchVelocity;
}

