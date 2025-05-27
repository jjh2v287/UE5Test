// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataTable.h"
#include "UKNeedComponent.generated.h"

UENUM(BlueprintType)
enum class EUKNeedType : uint8
{
	None	UMETA(DisplayName = "None"),
	Test	UMETA(DisplayName = "Test"),
	Max		UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FNeedDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Definition")
	EUKNeedType Type = EUKNeedType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Definition", meta=(ClampMin="0", ClampMax="1000"))
	int32 InitialValue = 50; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Definition", meta=(ClampMin="0", ClampMax="1000"))
	int32 MaxValue = 100; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Definition", meta=(ClampMin="0", ClampMax="1000"))
	int32 MinValue = 0; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Definition", meta=(Tooltip="초당 Need 값의 변화율 (양수: 증가, 음수: 감소)"))
	float DecayRate = 0.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Definition", meta=(ClampMin="0.0", ClampMax="1.0"))
	float Priority = 1.0f; 
};

USTRUCT(BlueprintType)
struct FNeedRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Needs")
	EUKNeedType Type = EUKNeedType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Needs")
	int32 CurrentValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs")
	int32 MaxValue = 100; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs")
	int32 MinValue = 0; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs")
	float DecayRate = 0.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs")
	float Priority = 1.0f; 

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Needs")
	float AccumulatedDecayRemainder = 0.0f; 

	FNeedRuntimeData()
	{}
	
	FNeedRuntimeData(const FNeedDefinition& Definition)
		: Type(Definition.Type), CurrentValue(Definition.InitialValue), MaxValue(Definition.MaxValue),
		  MinValue(Definition.MinValue), DecayRate(Definition.DecayRate), Priority(Definition.Priority),
		  AccumulatedDecayRemainder(0.0f)
	{}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ZKGAME_API UUKNeedComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UUKNeedComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void InitializeNeedData();
	
	UFUNCTION(BlueprintCallable, Category = "Needs|Debug")
	FString GetDebugString() const;
	
	UFUNCTION(BlueprintCallable, Category = "Needs")
	bool IncreaseNeed(const EUKNeedType NeedType, const int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Needs")
	bool DecreaseNeed(const EUKNeedType NeedType, const int32 Amount);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Needs")
	int32 GetNeedValue(const EUKNeedType NeedType) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Needs")
	FNeedRuntimeData GetNeedRuntimeData(const EUKNeedType NeedType) const;

	UFUNCTION(BlueprintCallable, Category = "Needs")
	FNeedRuntimeData GetHighestNeed() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Needs")
	float Evaluate(const EUKNeedType TargetNeedType) const;
	
protected:
	void UpdateNeeds(const float DeltaTime);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs|Configuration", meta=(RequiredAssetDataTags="RowStructure=NeedDefinition"))
	UDataTable* NeedDefinitionsTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Needs|CurrentState")
	TMap<EUKNeedType, FNeedRuntimeData> Needs;

	static constexpr int32 InvalidNeedValue = INDEX_NONE;
};
