// Fill out your copyright notice in the Description page of Project Settings.


#include "LandscapeTestActor.h"

#include "LandscapeProxy.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ALandscapeTestActor::ALandscapeTestActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALandscapeTestActor::BeginPlay()
{
	Super::BeginPlay();
	TArray<AActor*> Landscapes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALandscapeProxy::StaticClass(), Landscapes);

	// 씬에서 ALandscape(또는 ALandscapeProxy) 인스턴스 검색
	ALandscapeProxy* Landscape = Cast<ALandscapeProxy>(UGameplayStatics::GetActorOfClass(GetWorld(), ALandscapeProxy::StaticClass()));

	// 조회할 위치 설정 (Z값은 무시)
	FVector ddd = GetActorLocation();
	FVector QueryLoc(ddd.X, ddd.Y, 0.f);

	// 지형 높이 조회
	TOptional<float> HeightOpt = Landscape->GetHeightAtLocation(QueryLoc,EHeightfieldSource::Complex);
	if (Landscape && Landscape->GetComponentsBoundingBox().IsInside(ddd))
	{
		// 이 LandscapeProxy가 플레이어 아래에 있음.
		// Landscape에 맞는 추가 로직 작성
	}
	
	// if (HeightOpt.IsSet())
	// {
	// 	return HeightOpt.GetValue();  // 유효할 경우 Z 고도 반환 :contentReference[oaicite:6]{index=6}
	// }

	// if (bHit && HitResult.Actor->IsA(ALandscapeProxy::StaticClass()))
	// {
	// 	float LandscapeZ = HitResult.Location.Z;
	// 	// Z값 활용
	// }
}

// Called every frame
void ALandscapeTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

