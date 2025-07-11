// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Decorators/HTNDecorator_Cooldown.h"
#include "HTNPlan.h"
#include "Utility/HTNHelpers.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

UHTNDecorator_Cooldown::UHTNDecorator_Cooldown(const FObjectInitializer& Initializer) : Super(Initializer),
	bLockCooldown(true),
	bLockEvenIfExecutionAborted(true),
	CooldownDuration(5.0f),
	RandomDeviation(0.0f)
{
	bNotifyExecutionFinish = true;
}

FString UHTNDecorator_Cooldown::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	TStringBuilder<128> SB;

	if (bLockCooldown)
	{
		SB << TEXT("Cooldown");

		if (GameplayTag.IsValid())
		{
			SB << TEXT(" (") << GameplayTag.ToString() << TEXT(")");
		}
	}
	else
	{
		SB << (bInverseCondition ? TEXT("Is On Cooldown") : TEXT("Is Not On Cooldown"));

		if (GameplayTag.IsValid())
		{
			SB << TEXT(" (") << GameplayTag.ToString() << TEXT(")");
		}
		else
		{
			SB << TEXT(" (MISSING TAG!)");
		}
	}

	return SB.ToString();
}

FString UHTNDecorator_Cooldown::GetStaticDescription() const
{
	TStringBuilder<2048> StringBuilder;

	StringBuilder << Super::GetStaticDescription() << TEXT(":");

	if (bLockCooldown)
	{
		StringBuilder << TEXT("\nLock for ");
		StringBuilder << FString::SanitizeFloat(CooldownDuration);
		if (!FMath::IsNearlyZero(RandomDeviation))
		{
			StringBuilder << TEXT("+-") << FString::SanitizeFloat(RandomDeviation);
		}
		StringBuilder << TEXT("s after execution");

		if (!bLockEvenIfExecutionAborted)
		{
			StringBuilder << TEXT("\nWill not lock if execution was aborted");
		}

		if (GameplayTag.IsValid())
		{
			StringBuilder << TEXT("\nTag: ") << GameplayTag.ToString();
		}
		else
		{
			StringBuilder << TEXT("\nNo tag");
		}
	}
	else
	{
		StringBuilder << TEXT("\nCondition passes if the cooldown is ") <<
			(bInverseCondition ? TEXT("locked") : TEXT("unlocked"));

		if (GameplayTag.IsValid())
		{
			StringBuilder << TEXT("\nTag: ") << GameplayTag.ToString();
		}
		else
		{
			StringBuilder << TEXT("\n\nWARNING: missing gameplay tag.\nThe decorator checks if its cooldown is locked,\nbut since no gameplay tag is specified,\nnothing else ever locks this cooldown,\nso the condition is always the same.");
		}
	}

	return *StringBuilder;
}

uint16 UHTNDecorator_Cooldown::GetInstanceMemorySize() const
{
	return sizeof(FNodeMemory);
}

void UHTNDecorator_Cooldown::InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	FNodeMemory* const Memory = new(NodeMemory) FNodeMemory();
	if (const FHTNPlanStep* const Step = Plan.FindStep(StepID))
	{
		Memory->bIsIfNodeFalseBranch = Step->bIsIfNodeFalseBranch;
	}
}

bool UHTNDecorator_Cooldown::CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const
{
	const float CooldownEndTime = GetCooldownEndTime(OwnerComp);
	const bool bCooldownActive = UGameplayStatics::GetTimeSeconds(&OwnerComp) < CooldownEndTime;
	return !bCooldownActive;
}

void UHTNDecorator_Cooldown::OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result)
{
	if (bLockCooldown)
	{
		FNodeMemory* const Memory = CastInstanceNodeMemory<FNodeMemory>(NodeMemory);
		// If a Cooldown node is on an If node and the false branch is picked, it should not lock the cooldown upon execution finish.
		if (!Memory->bIsIfNodeFalseBranch)
		{
			if (Result != EHTNNodeResult::Aborted || bLockEvenIfExecutionAborted)
			{
				LockCooldown(OwnerComp);
			}
		}
	}
}

float UHTNDecorator_Cooldown::GetCooldownEndTime(UHTNComponent& OwnerComp) const
{
	if (const UHTNExtension_Cooldown* const Extension = OwnerComp.FindExtension<UHTNExtension_Cooldown>())
	{
		if (GameplayTag.IsValid())
		{
			return Extension->GetTagCooldownEndTime(GameplayTag);
		}
		else
		{
			return Extension->GetCooldownEndTime(this);
		}
	}

	return 0.0f;
}

void UHTNDecorator_Cooldown::LockCooldown(UHTNComponent& OwnerComp)
{
	UHTNExtension_Cooldown& Extension = OwnerComp.FindOrAddExtension<UHTNExtension_Cooldown>();
	
	const float EffectiveCooldownDuration = FMath::FRandRange(FMath::Max(0.0f, CooldownDuration - RandomDeviation), CooldownDuration + RandomDeviation);
	if (GameplayTag.IsValid())
	{
		Extension.AddTagCooldownDuration(GameplayTag, EffectiveCooldownDuration, /*bAddToExistingDuration=*/false);
	}
	else
	{
		Extension.AddCooldownDuration(this, EffectiveCooldownDuration, /*bAddToExistingDuration=*/false);
	}
}

void UHTNExtension_Cooldown::HTNStarted()
{
	CooldownOwnerToEndTime.Reset();
}

#if ENABLE_VISUAL_LOG

void UHTNExtension_Cooldown::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	struct Local
	{
		static FString GetTitle(const UObject* Object)
		{
			if (const UHTNNode* const Node = Cast<UHTNNode>(Object))
			{
				return Node->GetShortDescription();
			}

			return *GetNameSafe(Object);
		}
	};

	const float CurrentTime = UGameplayStatics::GetTimeSeconds(this);
	const auto GetTimeRemainingDescription = [&](const float EndTime) -> FString
	{
		const float TimeRemaining = EndTime - CurrentTime;
		if (TimeRemaining > 0.0f)
		{
			return FString::Printf(TEXT("%.2fs remaining"), TimeRemaining);
		}

		return TEXT("");
	};

	FVisualLogStatusCategory Category(TEXT("HTN Cooldowns"));

	for (const TPair<const UObject*, float>& Pair : CooldownOwnerToEndTime)
	{
		const UObject* const CooldownOwner = Pair.Key;
		const FString TimeRemainingDescription = GetTimeRemainingDescription(Pair.Value);
		if (IsValid(CooldownOwner) && !TimeRemainingDescription.IsEmpty())
		{
			Category.Add(*Local::GetTitle(CooldownOwner), TimeRemainingDescription);
		}
	}

	for (const TPair<FGameplayTag, float>& Pair : CooldownTagToEndTime)
	{
		const FString GameplayTagTimeRemainingDescription = GetTimeRemainingDescription(Pair.Value);
		if (!GameplayTagTimeRemainingDescription.IsEmpty())
		{
			Category.Add(Pair.Key.ToString(), GameplayTagTimeRemainingDescription);
		}
	}

	if (!Category.Data.Num())
	{
		Category.Add(TEXT("No active cooldowns"), TEXT(""));
	}

	Snapshot->Status.Add(MoveTemp(Category));
}

#endif

float UHTNExtension_Cooldown::GetCooldownEndTime(const UObject* CooldownOwner) const
{
	return CooldownOwnerToEndTime.FindRef(CooldownOwner);
}

float UHTNExtension_Cooldown::GetTagCooldownEndTime(const FGameplayTag& CooldownTag) const
{
	return CooldownTagToEndTime.FindRef(CooldownTag);
}

void UHTNExtension_Cooldown::AddCooldownDuration(const UObject* CooldownOwner, float CooldownDuration, bool bAddToExistingDuration)
{
	if (IsValid(CooldownOwner))
	{
		float* const CurrentEndTime = CooldownOwnerToEndTime.Find(CooldownOwner);
		if (bAddToExistingDuration && CurrentEndTime)
		{
			*CurrentEndTime += CooldownDuration;
		}
		else
		{
			CooldownOwnerToEndTime.Add(CooldownOwner, UGameplayStatics::GetTimeSeconds(this) + CooldownDuration);
		}
	}
}

void UHTNExtension_Cooldown::AddTagCooldownDuration(const FGameplayTag& CooldownTag, float CooldownDuration, bool bAddToExistingDuration)
{
	const TSharedPtr<FGameplayTagNode> TagNode = UGameplayTagsManager::Get().FindTagNode(CooldownTag);
	FHTNHelpers::ForGameplayTagAndChildTags(CooldownTag, [&](const FGameplayTag& Tag)
	{
		float* const CurrentEndTime = CooldownTagToEndTime.Find(Tag);
		if (bAddToExistingDuration && CurrentEndTime)
		{
			*CurrentEndTime += CooldownDuration;
		}
		else
		{
			CooldownTagToEndTime.Add(Tag, UGameplayStatics::GetTimeSeconds(this) + CooldownDuration);
		}
	});
}

void UHTNExtension_Cooldown::ResetCooldownsByTag(const FGameplayTag& CooldownTag)
{
	FHTNHelpers::ForGameplayTagAndChildTags(CooldownTag, [&](const FGameplayTag& Tag)
	{
		CooldownTagToEndTime.Remove(Tag);
	});
}

void UHTNExtension_Cooldown::ResetAllCooldownsWithoutGameplayTag()
{
	CooldownOwnerToEndTime.Reset();
}

void UHTNExtension_Cooldown::ResetAllCooldowns()
{
	CooldownOwnerToEndTime.Reset();
	CooldownTagToEndTime.Reset();
}
