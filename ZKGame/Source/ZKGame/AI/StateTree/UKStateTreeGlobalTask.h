// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once
#include "AIController.h"
#include "StateTreePropertyRef.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tasks/StateTreeAITask.h"
#include "UKStateTreeGlobalTask.generated.h"

/*USTRUCT()
struct FUKStateTreeGlobalTaskData
{
	GENERATED_BODY()

	/** Optional actor where to draw the text at. #1#
	UPROPERTY(EditAnywhere, Category = "Input", meta=(Optional))
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input", meta=(Optional))
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = Out, meta = (RefType = "bool"))
	FStateTreePropertyRef IsMove;

	// MovementComponent를 Instance Data로 이동
	UPROPERTY(Transient)
	TWeakObjectPtr<UCharacterMovementComponent> MovementComponent = nullptr;

	UPROPERTY(Transient)
	FTimerHandle TimerHandle;
};*/

UCLASS()
class ZKGAME_API UUKGlobalTaskData : public UObject
{
	GENERATED_BODY()
	
public:
	EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition);
	EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime);
	void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition);
	
protected:
	UFUNCTION()
	void OnHitCallback(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	EStateTreeRunStatus RunStatus = EStateTreeRunStatus::Running;

	UPROPERTY(EditAnywhere, Category = "Input", meta=(Optional))
	TObjectPtr<AAIController> AIController = nullptr;
};

USTRUCT(meta = (DisplayName = "Global Task", Category = "AI|Action"))
struct ZKGAME_API FUKStateTreeGlobalTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	// using FInstanceDataType = FUKStateTreeGlobalTaskData;
	// virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual const UStruct* GetInstanceDataType() const override { return UUKGlobalTaskData::StaticClass(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override
	{
		return Super::GetDescription(ID, InstanceDataView, BindingLookup, Formatting);
	}
	virtual FName GetIconName() const override
	{
		return FName("StateTreeEditorStyle|Node.Movement");
	}
	virtual FColor GetIconColor() const override
	{
		return UE::StateTree::Colors::Grey;
	}
#endif // WITH_EDITOR
};
