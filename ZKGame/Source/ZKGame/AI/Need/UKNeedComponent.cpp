// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKNeedComponent.h"
// #include "Common/UKCommonDefine.h"
// #include "Common/UKCommonLog.h"
#include "GameFramework/Actor.h"


UUKNeedComponent::UUKNeedComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bCanEverAffectNavigation = false;
}


void UUKNeedComponent::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeNeedData();
}

void UUKNeedComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateNeeds(DeltaTime);
}

void UUKNeedComponent::InitializeNeedData()
{
	Needs.Empty();

	if (!NeedDefinitionsTable)
	{
		// UK_LOG(Warning, "UUKNeedComponent: NeedDefinitionsTable is not set on %s.", *GetOwner()->GetName());
		return;
	}

	// Load all Need definitions from the Data Table
	TArray<FNeedDefinition*> OutRows;
	NeedDefinitionsTable->GetAllRows<FNeedDefinition>(TEXT("UUKNeedComponent::InitializeNeedData"), OutRows);

	for (FNeedDefinition* Row : OutRows)
	{
		if (Row && Row->Type != EUKNeedType::None && Row->Type != EUKNeedType::Max)
		{
			// Convert FNeedDefinition to FNeedRuntimeData and add to the map
			Needs.Add(Row->Type, FNeedRuntimeData(*Row));
		}
	}
}

FString UUKNeedComponent::GetDebugString() const
{
	FString DebugString = TEXT("UKNeedComponent Debug Info:\n");
    
	if (Needs.Num() == 0)
	{
		DebugString += TEXT("  No needs initialized");
		return DebugString;
	}
    
	DebugString += FString::Printf(TEXT("  Total Needs: %d\n"), Needs.Num());
	DebugString += TEXT("  Need Values:\n");

	for (TMap<EUKNeedType, FNeedRuntimeData>::TConstIterator It(Needs); It; ++It)
	{
		const EUKNeedType& NeedType = It.Key();
		const FNeedRuntimeData& NeedRuntimeData = It.Value();
		FString StatusText = TEXT("");
        
		// state indicator according to the value
		if (NeedRuntimeData.CurrentValue <= 0)
		{
			StatusText = TEXT(" (Critical)");
		}
		else if (NeedRuntimeData.CurrentValue <= 25)
		{
			StatusText = TEXT(" (Low)");
		}
		else if (NeedRuntimeData.CurrentValue <= 75)
		{
			StatusText = TEXT(" (Normal)");
		}
		else
		{
			StatusText = TEXT(" (High)");
		}
        
		DebugString += FString::Printf(TEXT("    %s: %d [%s]\n"), 
		   *UEnum::GetValueAsString(NeedType),
		   NeedRuntimeData.CurrentValue, 
		   *StatusText);
	}
    
	return DebugString;
}

bool UUKNeedComponent::IncreaseNeed(const EUKNeedType NeedType, const int32 Amount)
{
	FNeedRuntimeData* NeedData = Needs.Find(NeedType);
	if (!NeedData)
	{
		// UK_LOG(Warning, "UUKNeedComponent: Attempted to increase unknown NeedType '%s'", *UEnum::GetValueAsString(NeedType));	
		return false;
	}
	
	NeedData->CurrentValue = FMath::Clamp(NeedData->CurrentValue + Amount, NeedData->MinValue, NeedData->MaxValue);
	return true;
}

bool UUKNeedComponent::DecreaseNeed(const EUKNeedType NeedType, const int32 Amount)
{
	FNeedRuntimeData* NeedData = Needs.Find(NeedType);
	if (!NeedData)
	{
		// UK_LOG(Warning, "UUKNeedComponent: Attempted to decrease unknown NeedType '%s'", *UEnum::GetValueAsString(NeedType));
		return false;
	}
	
	NeedData->CurrentValue = FMath::Clamp(NeedData->CurrentValue - Amount, NeedData->MinValue, NeedData->MaxValue);
	return true;
}

int32 UUKNeedComponent::GetNeedValue(const EUKNeedType NeedType) const
{
	const FNeedRuntimeData* NeedData = Needs.Find(NeedType);
	if (!NeedData)
	{
		// UK_LOG(Warning, "UUKNeedComponent: Attempted to get value of unknown NeedType '%s'", *UEnum::GetValueAsString(NeedType));
		return InvalidNeedValue; 
	}
	
	return NeedData->CurrentValue;
}

FNeedRuntimeData UUKNeedComponent::GetNeedRuntimeData(const EUKNeedType NeedType) const
{
	const FNeedRuntimeData* NeedData = Needs.Find(NeedType);
	if (!NeedData)
	{
		// UK_LOG(Warning, "UUKNeedComponent: Attempted to get runtime data of unknown NeedType '%s'", *UEnum::GetValueAsString(NeedType));
		return FNeedRuntimeData();
	}
	
	return *NeedData;
}

float UUKNeedComponent::Evaluate(UUKNeedComponent* NeedsComponent, const EUKNeedType TargetNeedType)
{
	if (!NeedsComponent)
	{
		return 0.0f; 
	}
	
	if (TargetNeedType == EUKNeedType::None)
	{
		return 0.0f; 
	}

	FNeedRuntimeData NeedData = NeedsComponent->GetNeedRuntimeData(TargetNeedType);
	if (NeedData.Type == EUKNeedType::None || NeedData.MaxValue == NeedData.MinValue) 
	{
		return 0.0f;
	}

	// 현재 Need 값을 0.0 ~ 1.0 사이로 정규화.
	float NormalizedNeedValue = 0.0f;
	if (NeedData.MaxValue - NeedData.MinValue > 0)
	{
		NormalizedNeedValue = (float)(NeedData.CurrentValue - NeedData.MinValue) / (float)(NeedData.MaxValue - NeedData.MinValue);
	}

	// 중요도
	float WeightedScore = NormalizedNeedValue * NeedData.Priority;

	// 점수를 0.0 ~ 1.0 사이로 클램프
	WeightedScore = FMath::Clamp(WeightedScore, 0.0f, 1.0f);

	return WeightedScore;
}

void UUKNeedComponent::UpdateNeeds(const float DeltaTime)
{
	for (auto& Pair : Needs)
	{
		FNeedRuntimeData& NeedData = Pair.Value;
		if (NeedData.DecayRate != 0.0f)
		{
			NeedData.AccumulatedDecayRemainder += NeedData.DecayRate * DeltaTime;

			int32 IntegerChange = 0;
			if (NeedData.DecayRate > 0)
			{
				// 소수점 버리고 정수 부분만 추출
				IntegerChange = FMath::FloorToInt(NeedData.AccumulatedDecayRemainder);
			}
			else
			{
				// 음수일 때 -1.5 -> -1, -0.5 -> 0
				IntegerChange = FMath::CeilToInt(NeedData.AccumulatedDecayRemainder);
			}

			if (IntegerChange != 0)
			{
				// 추출된 정수 변화량을 현재 값에 더하고 Clamp 적용
				NeedData.CurrentValue = FMath::Clamp(NeedData.CurrentValue + IntegerChange, NeedData.MinValue, NeedData.MaxValue);
				// 적용된 정수만큼 누적된 소수점 잔여량에서 차감 1.08f -> 0.08f
				NeedData.AccumulatedDecayRemainder -= IntegerChange; 
			}
		}
	}
}