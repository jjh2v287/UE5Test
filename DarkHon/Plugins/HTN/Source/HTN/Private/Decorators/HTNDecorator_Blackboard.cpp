// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Decorators/HTNDecorator_Blackboard.h"
#include "WorldStateProxy.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_NativeEnum.h"
#include "HTNPlan.h"

UHTNDecorator_Blackboard::UHTNDecorator_Blackboard(const FObjectInitializer& Initializer) : Super(Initializer),
	bCanAbortPlanInstantly(true)
{
	// If bCheckConditionOnTick is enabled, then we only want to actually check on the first tick, and then rely on blackboard events to handle condition changes.
	bCheckConditionOnTickOnlyOnce = true;
}

FString UHTNDecorator_Blackboard::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	return CachedDescription;
}

FString UHTNDecorator_Blackboard::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s%s"), *Super::GetStaticDescription(), *CachedDescription, 
		bCheckConditionOnTick ? TEXT("\nNotifies when condition changes") : TEXT(""));
}

bool UHTNDecorator_Blackboard::CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const
{
	if (UWorldStateProxy* const WorldStateProxy = GetWorldStateProxy(OwnerComp, CheckType))
	{
		return EvaluateConditionOnWorldState(*WorldStateProxy);
	}

	return true;
}

#if WITH_EDITOR

void UHTNDecorator_Blackboard::BuildDescription()
{
	static UEnum* const BasicOpEnum = StaticEnum<EBasicKeyOperation::Type>();
	static UEnum* const ArithmeticOpEnum = StaticEnum<EArithmeticKeyOperation::Type>();
	static UEnum* const TextOpEnum = StaticEnum<ETextKeyOperation::Type>();
	
	const UBlackboardData* const BlackboardAsset = GetBlackboardAsset();
	const FBlackboardEntry* const EntryInfo = BlackboardAsset ? BlackboardAsset->GetKey(BlackboardKey.GetSelectedKeyID()) : nullptr;

	FString BlackboardDesc = TEXT("invalid");
	if (EntryInfo)
	{
		// safety feature to not crash when changing couple of properties on a bunch
		// while "post edit property" triggers for every each of them
		if (EntryInfo->KeyType->GetClass() == BlackboardKey.SelectedKeyType)
		{
			const FString KeyName = EntryInfo->EntryName.ToString();

			const EBlackboardKeyOperation::Type Op = EntryInfo->KeyType->GetTestOperation();
			switch (Op)
			{
			case EBlackboardKeyOperation::Basic:
				BlackboardDesc = FString::Printf(TEXT("%s %s"), 
					*KeyName, 
					*BasicOpEnum->GetDisplayNameTextByValue(OperationType).ToString()
				);
				break;

			case EBlackboardKeyOperation::Arithmetic:
				BlackboardDesc = FString::Printf(TEXT("%s %s %s"), 
					*KeyName, *ArithmeticOpEnum->GetDisplayNameTextByValue(OperationType).ToString(),
					*EntryInfo->KeyType->DescribeArithmeticParam(IntValue, FloatValue)
				);
				break;

			case EBlackboardKeyOperation::Text:
				BlackboardDesc = FString::Printf(TEXT("%s %s [%s]"), 
					*KeyName, 
					*TextOpEnum->GetDisplayNameTextByValue(OperationType).ToString(), 
					*StringValue
				);
				break;

			default:
				break;
			}
		}
	}

	CachedDescription = BlackboardDesc;
}

void UHTNDecorator_Blackboard::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	const FName ChangedPropName = PropertyChangedEvent.Property->GetFName();
	if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UHTNDecorator_Blackboard, BlackboardKey.SelectedKeyName))
	{
		if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Enum::StaticClass() ||
			BlackboardKey.SelectedKeyType == UBlackboardKeyType_NativeEnum::StaticClass())
		{
			IntValue = 0;
		}
	}

#if WITH_EDITORONLY_DATA

	UBlackboardKeyType* const KeyCDO = BlackboardKey.SelectedKeyType ? BlackboardKey.SelectedKeyType->GetDefaultObject<UBlackboardKeyType>() : nullptr;
	if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UHTNDecorator_Blackboard, BasicOperation))
	{
		if (KeyCDO && KeyCDO->GetTestOperation() == EBlackboardKeyOperation::Basic)
		{
			OperationType = BasicOperation;
		}
	}
	else if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UHTNDecorator_Blackboard, ArithmeticOperation))
	{
		if (KeyCDO && KeyCDO->GetTestOperation() == EBlackboardKeyOperation::Arithmetic)
		{
			OperationType = ArithmeticOperation;
		}
	}
	else if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UHTNDecorator_Blackboard, TextOperation))
	{
		if (KeyCDO && KeyCDO->GetTestOperation() == EBlackboardKeyOperation::Text)
		{
			OperationType = TextOperation;
		}
	}

#endif

	BuildDescription();
}

void UHTNDecorator_Blackboard::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);
	BuildDescription();
}

#endif

EBlackboardNotificationResult UHTNDecorator_Blackboard::OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID, uint8* NodeMemory)
{
	UHTNComponent* const HTNComponent = Cast<UHTNComponent>(Blackboard.GetBrainComponent());
	if (!HTNComponent)
	{
		return EBlackboardNotificationResult::RemoveObserver;
	}

	// Only if condition checks on tick (aka during plan execution) are enabled.
	if (bCheckConditionOnTick && ChangedKeyID == BlackboardKey.GetSelectedKeyID())
	{
		const bool bRawConditionValue = EvaluateConditionOnWorldState(*HTNComponent->GetBlackboardProxy());
		if (NotifyEventBasedCondition(*HTNComponent, NodeMemory, bRawConditionValue, bCanAbortPlanInstantly))
		{
			return EBlackboardNotificationResult::RemoveObserver;
		}
	}

	return EBlackboardNotificationResult::ContinueObserving;
}

bool UHTNDecorator_Blackboard::EvaluateConditionOnWorldState(const UWorldStateProxy& WorldStateProxy) const
{
	bool bResult = false;
	if (BlackboardKey.SelectedKeyType)
	{
		const UBlackboardKeyType* const KeyCDO = BlackboardKey.SelectedKeyType->GetDefaultObject<UBlackboardKeyType>();
		const FBlackboard::FKey KeyID = BlackboardKey.GetSelectedKeyID();
		if (ensure(KeyCDO))
		{
			const EBlackboardKeyOperation::Type Op = KeyCDO->GetTestOperation();
			switch (Op)
			{
			case EBlackboardKeyOperation::Basic:
				bResult = WorldStateProxy.TestBasicOperation(*KeyCDO, KeyID, StaticCast<EBasicKeyOperation::Type>(OperationType));
				break;

			case EBlackboardKeyOperation::Arithmetic:
				bResult = WorldStateProxy.TestArithmeticOperation(*KeyCDO, KeyID, StaticCast<EArithmeticKeyOperation::Type>(OperationType), IntValue, FloatValue);
				break;

			case EBlackboardKeyOperation::Text:
				bResult = WorldStateProxy.TestTextOperation(*KeyCDO, KeyID, StaticCast<ETextKeyOperation::Type>(OperationType), StringValue);
				break;

			default:
				break;
			}
		}
	}

	return bResult;
}
