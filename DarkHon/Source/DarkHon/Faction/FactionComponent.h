// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FactionComponent.generated.h"

UENUM(BlueprintType)
enum class EFaction : uint8
{
	Player      UMETA(DisplayName = "Player"),
	Goblin      UMETA(DisplayName = "Goblin"),
	Slime       UMETA(DisplayName = "Slime"),
	Test		UMETA(DisplayName = "Test"),
	Orc			UMETA(DisplayName = "Orc")
};

USTRUCT(BlueprintType)
struct FFactionDataTable : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction Type")
	EFaction Faction = EFaction::Player;
	
	// 이 팩션이 다른 팩션에 대해 느끼는 위협도를 저장하는 맵
	// Key: 상대방 팩션, Value: 위협도 (높을수록 적대적)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Threat Map")
	TMap<EFaction, int32> ThreatMap;

	// 다른 액터에 대한 위협도를 반환합니다.
	static int32 GetThreatLevelTowards(AActor* SourceActor, AActor* OtherActor);

	// 특정 팩션에 대한 위협도를 반환합니다.
	static int32 QueryThreatLevel(EFaction SourceFaction, EFaction TargetFaction);

private:
	// 기본 위협도 (데이터 테이블에 명시되지 않은 경우)
	static constexpr int32 DefaultThreat = 0;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DARKHON_API UFactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFactionComponent();
	virtual void BeginPlay() override;

	EFaction GetFaction() const
	{
		return Faction;
	}
	
protected:
	EFaction Faction = EFaction::Player;
};
