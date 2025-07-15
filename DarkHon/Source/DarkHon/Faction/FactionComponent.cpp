// Fill out your copyright notice in the Description page of Project Settings.

#include "FactionComponent.h"


UFactionComponent::UFactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFactionComponent::BeginPlay()
{
	Super::BeginPlay();
	// 데이터에서 팩션 설정
	// Faction = 
}

int32 FFactionDataTable::GetThreatLevelTowards(AActor* SourceActor, AActor* OtherActor)
{
	if (!SourceActor || !OtherActor)
	{
		return DefaultThreat;
	}

	UFactionComponent* SourceFactionComponent = SourceActor->FindComponentByClass<UFactionComponent>();
	if (!SourceFactionComponent)
	{
		return DefaultThreat;
	}

	UFactionComponent* OtherFactionComponent = OtherActor->FindComponentByClass<UFactionComponent>();
	if (!OtherFactionComponent)
	{
		return DefaultThreat;
	}

	// 팩션 대 팩션으로 위협도를 조회합니다.
	const EFaction SourceFaction = SourceFactionComponent->GetFaction();
	const EFaction OtherFaction = SourceFactionComponent->GetFaction();
	return QueryThreatLevel(SourceFaction, OtherFaction);
}

int32 FFactionDataTable::QueryThreatLevel(EFaction SourceFaction, EFaction TargetFaction)
{
	if (SourceFaction == TargetFaction)
	{
		return DefaultThreat;
	}
	
	/*if (!FactionDataTable )
	{
		return DefaultThreat;
	}
    
	// Enum 값을 문자열로 변환하여 행 이름으로 사용합니다.
	const FString ContextString(TEXT("Faction Data Context"));
	const FName RowName = *UEnum::GetValueAsString(SourceFaction);

	// 데이터 테이블에서 SourceFaction에 해당하는 행을 찾습니다.
	const FFactionDataTable* RowData = FactionDataTable->FindRow<FFactionDataTable>(RowName, ContextString);

	if (RowData)
	{
		// 행 데이터의 ThreatMap에서 TargetFaction에 대한 위협도를 찾습니다.
		if (const float* FoundThreat = RowData->Faction.Find(TargetFaction))
		{
			return *FoundThreat;
		}
	}*/
    
	// 행이나 맵에 값이 없는 경우 기본값을 반환합니다.
	return DefaultThreat;
}