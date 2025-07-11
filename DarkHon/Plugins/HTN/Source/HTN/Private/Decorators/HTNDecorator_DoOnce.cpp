// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Decorators/HTNDecorator_DoOnce.h"
#include "HTNPlan.h"
#include "Utility/HTNHelpers.h"

#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

UHTNDecorator_DoOnce::UHTNDecorator_DoOnce(const FObjectInitializer& Initializer) : Super(Initializer),
	GameplayTag(FGameplayTag::EmptyTag),
	bLockEvenIfExecutionAborted(true),
	bCanAbortPlanInstantly(true)
{
	bNotifyExecutionStart = true;
	bNotifyExecutionFinish = true;

	// Because we want to check the condition during execution in an event-based way instead of every tick.
	bCheckConditionOnTickOnlyOnce = true;
}

FString UHTNDecorator_DoOnce::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	if (GameplayTag.IsValid())
	{
		return FString::Printf(TEXT("Do Once (%s)"), *GameplayTag.ToString());
	}
	else
	{
		return TEXT("Do Once");
	}
}

FString UHTNDecorator_DoOnce::GetStaticDescription() const
{
	TStringBuilder<2048> StringBuilder;
	StringBuilder << Super::GetStaticDescription() << TEXT(":");

	if (GameplayTag.IsValid())
	{
		StringBuilder << TEXT("\nTag: ") << GameplayTag.ToString();
	}
	else
	{
		StringBuilder << TEXT("\nNo tag");
	}

	if (bCheckConditionOnTick)
	{
		StringBuilder << TEXT("\nNotifies when condition changes");
	}

	if (!bLockEvenIfExecutionAborted)
	{
		StringBuilder << TEXT("\nWill not lock if execution was aborted");
	}

	return *StringBuilder;
}

uint16 UHTNDecorator_DoOnce::GetInstanceMemorySize() const
{
	return sizeof(FNodeMemory);
}

void UHTNDecorator_DoOnce::InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	FNodeMemory* const Memory = new(NodeMemory) FNodeMemory;
	if (const FHTNPlanStep* const Step = Plan.FindStep(StepID))
	{
		Memory->bIsIfNodeFalseBranch = Step->bIsIfNodeFalseBranch;
	}
}

bool UHTNDecorator_DoOnce::IsLockableByGameplayTag() const
{
	return GameplayTag.IsValid();
}

bool UHTNDecorator_DoOnce::CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const
{
	UHTNExtension_DoOnce& Extension = OwnerComp.FindOrAddExtension<UHTNExtension_DoOnce>();
	const bool bIsLocked = Extension.IsDoOnceLocked(this);
	return !bIsLocked;
}

void UHTNDecorator_DoOnce::OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory)
{
	UHTNExtension_DoOnce& Extension = OwnerComp.FindOrAddExtension<UHTNExtension_DoOnce>();
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);

	if (bCheckConditionOnTick)
	{
		if (IsLockableByGameplayTag())
		{
			Memory.OnGameplayTagLockStatusChangedHandle = Extension.OnGameplayTagLockStatusChangedDelegate
				.AddUObject(this, &UHTNDecorator_DoOnce::OnGameplayTagLockStatusChanged, NodeMemory);
		}
		else
		{
			Memory.OnDecoratorLockStatusChangedHandle = Extension.OnDecoratorLockStatusChangedDelegate
				.AddUObject(this, &UHTNDecorator_DoOnce::OnDecoratorLockStatusChanged, NodeMemory);
		}
	}
}

void UHTNDecorator_DoOnce::OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result)
{
	UHTNExtension_DoOnce& Extension = OwnerComp.FindOrAddExtension<UHTNExtension_DoOnce>();
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);

	if (Memory.OnGameplayTagLockStatusChangedHandle.IsValid())
	{
		Extension.OnGameplayTagLockStatusChangedDelegate.Remove(Memory.OnGameplayTagLockStatusChangedHandle);
		Memory.OnGameplayTagLockStatusChangedHandle.Reset();
	}

	if (Memory.OnDecoratorLockStatusChangedHandle.IsValid())
	{
		Extension.OnDecoratorLockStatusChangedDelegate.Remove(Memory.OnDecoratorLockStatusChangedHandle);
		Memory.OnDecoratorLockStatusChangedHandle.Reset();
	}

	// If the DoOnce decorator is on an If node and the false branch was picked, it should not lock upon execution finish.
	if (!Memory.bIsIfNodeFalseBranch)
	{
		if (Result != EHTNNodeResult::Aborted || bLockEvenIfExecutionAborted)
		{
			Extension.SetDoOnceLocked(this);
		}
	}
}

void UHTNDecorator_DoOnce::OnGameplayTagLockStatusChanged(UHTNExtension_DoOnce& Sender, const FGameplayTag& ChangedTag, bool bNewLocked, uint8* NodeMemory)
{
	if (ChangedTag == GameplayTag)
	{
		if (UHTNComponent* const HTNComponent = Sender.GetHTNComponent())
		{
			NotifyEventBasedCondition(*HTNComponent, NodeMemory, !bNewLocked, bCanAbortPlanInstantly);
		}
	}
}

void UHTNDecorator_DoOnce::OnDecoratorLockStatusChanged(UHTNExtension_DoOnce& Sender, const UHTNDecorator_DoOnce* Decorator, bool bNewLocked, uint8* NodeMemory)
{
	// If this decorator is present multiple times inside a recursive HTN, 
	// this allows a higher-level instance of this in the plan to abort when the lower-level one locks itself.
	if (this == Decorator)
	{
		if (UHTNComponent* const HTNComponent = Sender.GetHTNComponent())
		{
			NotifyEventBasedCondition(*HTNComponent, NodeMemory, !bNewLocked, bCanAbortPlanInstantly);
		}
	}
}

void UHTNExtension_DoOnce::Cleanup()
{
	ResetAllDoOnceDecorators();
}

#if ENABLE_VISUAL_LOG

void UHTNExtension_DoOnce::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category(TEXT("HTN Do Once"));

	Category.Add(FString::Printf(TEXT("Locked Gameplay Tags")), FString::FromInt(LockedGameplayTags.Num()));
	for (const FGameplayTag& Tag : LockedGameplayTags)
	{
		Category.Add(*Tag.ToString(), TEXT(""));
	}

	Category.Add(FString::Printf(TEXT("Locked Decorators")), FString::FromInt(LockedDecorators.Num()));
	for (const UHTNDecorator_DoOnce* const Decorator : LockedDecorators)
	{
		if (IsValid(Decorator))
		{
			Category.Add(*Decorator->GetNodeName(), *GetNameSafe(Decorator->GetSourceHTNAsset()));
		}
	}

	if (!Category.Data.Num())
	{
		Category.Add(TEXT("No locked Do Once decorators"), TEXT(""));
	}

	Snapshot->Status.Add(MoveTemp(Category));
}

#endif

bool UHTNExtension_DoOnce::IsDoOnceLocked(const UHTNDecorator_DoOnce* Decorator) const
{
	if (!IsValid(Decorator))
	{
		return false;
	}

	if (Decorator->IsLockableByGameplayTag())
	{
		return IsDoOnceLocked(Decorator->GameplayTag);
	}
	else
	{
		return LockedDecorators.Contains(Decorator);
	}
}

bool UHTNExtension_DoOnce::IsDoOnceLocked(const FGameplayTag& Tag) const
{
	return LockedGameplayTags.Contains(Tag);
}

bool UHTNExtension_DoOnce::SetDoOnceLocked(const UHTNDecorator_DoOnce* Decorator, bool bNewLocked)
{
	if (!IsValid(Decorator))
	{
		return false;
	}

	if (Decorator->IsLockableByGameplayTag())
	{
		return SetDoOnceLocked(Decorator->GameplayTag, bNewLocked);
	}

	bool bChanged = true;
	if (bNewLocked)
	{
		bool bUnchanged = true;
		LockedDecorators.Add(Decorator, &bUnchanged);
		bChanged = !bUnchanged;
	}
	else
	{
		bChanged = LockedDecorators.Remove(Decorator) > 0;
	}

	if (bChanged)
	{
		OnDecoratorLockStatusChangedDelegate.Broadcast(*this, Decorator, bNewLocked);
	}

	return bChanged;
}

bool UHTNExtension_DoOnce::SetDoOnceLocked(const FGameplayTag& Tag, bool bNewLocked)
{
	bool bChanged = false;
	if (bNewLocked)
	{
		FHTNHelpers::ForGameplayTagAndChildTags(Tag, [&](const FGameplayTag& ChildTag)
		{
			bool bTagAtNodeUnchanged = true;
			LockedGameplayTags.Add(ChildTag, &bTagAtNodeUnchanged);
			bChanged |= !bTagAtNodeUnchanged;

			if (!bTagAtNodeUnchanged)
			{
				OnGameplayTagLockStatusChangedDelegate.Broadcast(*this, ChildTag, bNewLocked);
			}
		});
	}
	else
	{
		FHTNHelpers::ForGameplayTagAndChildTags(Tag, [&](const FGameplayTag& ChildTag)
		{
			bool bTagAtNodeChanged = LockedGameplayTags.Remove(ChildTag) > 0;
			bChanged |= bTagAtNodeChanged;

			if (bTagAtNodeChanged)
			{
				OnGameplayTagLockStatusChangedDelegate.Broadcast(*this, ChildTag, bNewLocked);
			}
		});
	}

	return bChanged;
}

void UHTNExtension_DoOnce::ResetAllDoOnceDecoratorsWithoutGameplayTag()
{
	// We need to actually unlock all decorators before propagating events about this, in case these events trigger a synchronous replan.
	// Once new planning begins, it should know that all the decorators are now unlocked, not just the ones we've iterated over so far.

	TSet<TObjectPtr<const UHTNDecorator_DoOnce>> ResetDecorators;
	Swap(LockedDecorators, ResetDecorators);

	for (const UHTNDecorator_DoOnce* const ResetDecorator : ResetDecorators)
	{
		OnDecoratorLockStatusChangedDelegate.Broadcast(*this, ResetDecorator, /*bNewLocked=*/false);
	}
}

void UHTNExtension_DoOnce::ResetAllDoOnceDecorators()
{
	TSet<FGameplayTag> ResetGameplayTags;
	Swap(LockedGameplayTags, ResetGameplayTags);

	TSet<TObjectPtr<const UHTNDecorator_DoOnce>> ResetDecorators;
	Swap(LockedDecorators, ResetDecorators);

	for (const UHTNDecorator_DoOnce* const ResetDecorator : ResetDecorators)
	{
		OnDecoratorLockStatusChangedDelegate.Broadcast(*this, ResetDecorator, /*bNewLocked=*/false);
	}

	for (const FGameplayTag& ResetGameplayTag : ResetGameplayTags)
	{
		OnGameplayTagLockStatusChangedDelegate.Broadcast(*this, ResetGameplayTag, /*bNewLocked=*/false);
	}
}
