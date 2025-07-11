// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Decorators/HTNDecorator_BlackboardBase.h"
#include "AISystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "VisualLogger/VisualLogger.h"

UHTNDecorator_BlackboardBase::UHTNDecorator_BlackboardBase(const FObjectInitializer& Initializer) : Super(Initializer),
	bNotifyOnBlackboardKeyValueChange(true)
{
	bNotifyExecutionStart = true;
	bNotifyExecutionFinish = true;

	BlackboardKey.AllowNoneAsValue(GET_AI_CONFIG_VAR(bBlackboardKeyDecoratorAllowsNoneAsValue));
}

void UHTNDecorator_BlackboardBase::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* const BBAsset = GetBlackboardAsset())
	{
		BlackboardKey.ResolveSelectedKey(*BBAsset);
	}
	else
	{
		UE_LOG(LogHTN, Warning, TEXT("Can't initialize %s due to missing blackboard data."), *GetShortDescription());
		BlackboardKey.InvalidateResolvedKey();
	}
}

uint16 UHTNDecorator_BlackboardBase::GetSpecialMemorySize() const
{
	return sizeof(FHTNDecoratorBlackboardBaseSpecialMemory);
}

#if WITH_EDITOR

FName UHTNDecorator_BlackboardBase::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Blackboard.Icon");
}

#endif

void UHTNDecorator_BlackboardBase::OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory)
{
	if (bNotifyOnBlackboardKeyValueChange)
	{
		FHTNDecoratorBlackboardBaseSpecialMemory* const SpecialMemory = GetSpecialNodeMemory<FHTNDecoratorBlackboardBaseSpecialMemory>(NodeMemory);
		if (ensure(SpecialMemory))
		{
			UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
			if (IsValid(Blackboard))
			{
				if (!SpecialMemory->OnBlackboardKeyValueChangeHandle.IsValid())
				{
					SpecialMemory->OnBlackboardKeyValueChangeHandle = Blackboard->RegisterObserver(BlackboardKey.GetSelectedKeyID(), this,
						FOnBlackboardChangeNotification::CreateUObject(this, &UHTNDecorator_BlackboardBase::OnBlackboardKeyValueChange, NodeMemory));
				}
				else
				{
					UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("%s: OnBlackboardKeyValueChangeHandle already valid OnExecutionStart"), *GetShortDescription());
				}
			}
		}
	}
}

void UHTNDecorator_BlackboardBase::OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result)
{
	if (bNotifyOnBlackboardKeyValueChange)
	{
		FHTNDecoratorBlackboardBaseSpecialMemory* const SpecialMemory = GetSpecialNodeMemory<FHTNDecoratorBlackboardBaseSpecialMemory>(NodeMemory);
		if (ensure(SpecialMemory))
		{
			UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
			if (IsValid(Blackboard))
			{
				if (SpecialMemory->OnBlackboardKeyValueChangeHandle.IsValid())
				{
					Blackboard->UnregisterObserver(BlackboardKey.GetSelectedKeyID(), SpecialMemory->OnBlackboardKeyValueChangeHandle);
					SpecialMemory->OnBlackboardKeyValueChangeHandle.Reset();
				}
				else
				{
					UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("%s: OnBlackboardKeyValueChangeHandle is not valid OnExecutionFinish"), *GetShortDescription());
				}
			}
		}
	}
}

EBlackboardNotificationResult UHTNDecorator_BlackboardBase::OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID, uint8* NodeMemory)
{
	return Cast<UHTNComponent>(Blackboard.GetBrainComponent()) ? 
		EBlackboardNotificationResult::ContinueObserving :
		EBlackboardNotificationResult::RemoveObserver;
}
