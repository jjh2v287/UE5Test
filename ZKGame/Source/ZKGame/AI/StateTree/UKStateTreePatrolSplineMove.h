// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/CharacterMovementComponent.h"
#include "Tasks/StateTreeAITask.h"
#include "UKStateTreePatrolSplineMove.generated.h"

USTRUCT()
struct FUKStateTreePatrolSplineMoveData
{
	GENERATED_BODY()

	/** Optional actor where to draw the text at. */
	UPROPERTY(EditAnywhere, Category = "Input", meta=(Optional))
	TObjectPtr<AActor> ReferenceActor = nullptr;

	// MovementComponent를 Instance Data로 이동
	UPROPERTY(Transient)
	TWeakObjectPtr<UCharacterMovementComponent> MovementComponent = nullptr;
};

USTRUCT(meta = (DisplayName = "Patrol Spline Move", Category = "AI|Action"))
struct FUKStateTreePatrolSplineMove : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FUKStateTreePatrolSplineMoveData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	
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
