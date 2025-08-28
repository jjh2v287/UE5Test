// Fill out your copyright notice in the Description page of Project Settings.


#include "LandscapeTestActor.h"
#include "Engine/World.h"
#include "LandscapeProxy.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ALandscapeTestActor::ALandscapeTestActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

#if WITH_EDITOR
void ALandscapeTestActor::PostActorCreated()
{
	Super::PostActorCreated();
	if (!PersistentId.IsValid())
	{
		PersistentId = FGuid::NewGuid();
		Modify(); // 외부액터 파일 더티 → 저장 시 직렬화
	}
	
	ActorUID = FName(*GetStableKey(this));
}

void ALandscapeTestActor::PostLoad()
{
	Super::PostLoad();
	if (!PersistentId.IsValid())
	{
		PersistentId = FGuid::NewGuid();
		Modify();
	}

	ActorUID = FName(*GetStableKey(this));
}
#endif

// Called when the game starts or when spawned
void ALandscapeTestActor::BeginPlay()
{
	Super::BeginPlay();
	// 에디터 배치 액터라면 수동으로 GUID를 넣어둘 수도 있고,
	// 런타임 스폰이라면 BeginPlay에서 보장해도 됩니다.
	if (!PersistentId.IsValid())
	{
		PersistentId = FGuid::NewGuid(); // 안정적 영속 ID 발급
	}
	
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

FString ALandscapeTestActor::GetStableKey(const AActor* A)
{
	/*// 대안 키: 외부 패키지/객체 이름 기반 (배치 액터용으로 안정)
	if (const UPackage* Pkg = A ? A->GetOutermost() : nullptr)
	{
		return Pkg->GetFName();
	}
	
	return A ? A->GetFName() : NAME_None;*/
	
	return UKStringUtil::GetActorUID(A);
}

// Called every frame
void ALandscapeTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (PersistentId.IsValid())
	{
		DrawDebugString(GetWorld(), GetActorLocation(), PersistentId.ToString(), this, FColor::Black, -1, false, 12);
	}
}

