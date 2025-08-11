// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LandscapeTestActor.generated.h"

UCLASS()
class DARKHON_API ALandscapeTestActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALandscapeTestActor();

#if WITH_EDITOR
	virtual void PostActorCreated() override; // 에디터에서 새로 배치/복제
	virtual void PostLoad() override;         // 디스크 로드(에디터/쿠킹 경로)
#endif
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="Stream")
	static FName GetStableKey(const AActor* A); // 대안: 패키지명 키

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stream")
	FGuid PersistentId; // 에디터 배치 시 미리 지정 가능, 런타임 스폰 시 NewGuid()
};
