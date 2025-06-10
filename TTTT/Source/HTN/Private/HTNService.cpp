// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNService.h"

#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

FHTNServiceSpecialMemory::FHTNServiceSpecialMemory(float Interval) :
	TickCountdown(Interval),
	bFirstTick(true)
{}

UHTNService::UHTNService(const FObjectInitializer& Initializer) : Super(Initializer),
	TickInterval(0.5f),
	TickIntervalRandomDeviation(0.1f),
	bUsePersistentTickCountdown(false),
	bNotifyExecutionStart(false),
	bNotifyTick(false),
	bNotifyExecutionFinish(false)
{}

FString UHTNService::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s:\n%s"), *Super::GetStaticDescription(), *GetStaticServiceDescription());
}

uint16 UHTNService::GetSpecialMemorySize() const
{
	return sizeof(FHTNServiceSpecialMemory);
}

void UHTNService::InitializeSpecialMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	new(GetSpecialNodeMemory<FHTNServiceSpecialMemory>(NodeMemory)) FHTNServiceSpecialMemory(GetInterval());
}

void UHTNService::WrappedExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) const
{
	UHTNService* const Service = StaticCast<UHTNService*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (!ensure(Service))
	{
		return;
	}

	if (bNotifyTick && bUsePersistentTickCountdown)
	{
		const UHTNExtension_ServicePersistentTickCountdown& Extension = OwnerComp.FindOrAddExtension<UHTNExtension_ServicePersistentTickCountdown>();
		FHTNServiceSpecialMemory& SpecialMemory = *GetSpecialNodeMemory<FHTNServiceSpecialMemory>(NodeMemory);
		SpecialMemory.bFirstTick = !Extension.GetPersistentTickCountdown(SpecialMemory.TickCountdown, this);
	}
	
	UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("service execution start: '%s'"), *Service->GetShortDescription());
	if (bNotifyExecutionStart)
	{
		Service->OnExecutionStart(OwnerComp, NodeMemory);
	}
}

void UHTNService::WrappedTickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	UHTNService* const Service = StaticCast<UHTNService*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (!ensure(Service))
	{
		return;
	}
	
	if (Service->bNotifyTick)
	{
		FHTNServiceSpecialMemory& SpecialMemory = *GetSpecialNodeMemory<FHTNServiceSpecialMemory>(NodeMemory);
		FIntervalCountdown& TickCountdown = SpecialMemory.TickCountdown;
		if (TickCountdown.Tick(DeltaSeconds))
		{
			UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("service tick %s"), *Service->GetShortDescription());

			DeltaSeconds = SpecialMemory.bFirstTick ? 
				DeltaSeconds : 
				TickCountdown.GetElapsedTimeWithFallback(DeltaSeconds);

			Service->TickNode(OwnerComp, NodeMemory, DeltaSeconds);

			SpecialMemory.bFirstTick = false;
			TickCountdown.Interval = GetInterval();
			TickCountdown.Reset();
		}
	}
}

void UHTNService::WrappedExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult NodeResult) const
{
	UHTNService* const Service = StaticCast<UHTNService*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (!ensure(Service))
	{
		return;
	}

	if (bNotifyTick && bUsePersistentTickCountdown)
	{
		UHTNExtension_ServicePersistentTickCountdown& Extension = OwnerComp.FindOrAddExtension<UHTNExtension_ServicePersistentTickCountdown>();
		const FHTNServiceSpecialMemory& SpecialMemory = *GetSpecialNodeMemory<FHTNServiceSpecialMemory>(NodeMemory);
		Extension.SetPersistentTickCountdown(this, SpecialMemory.TickCountdown);
	}
	
	UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("service execution finish: '%s'"), *Service->GetShortDescription());
	if (Service->bNotifyExecutionFinish)
	{
		Service->OnExecutionFinish(OwnerComp, NodeMemory, NodeResult);
	}
}

FString UHTNService::GetStaticServiceDescription() const
{
	return GetStaticTickIntervalDescription();
}

FString UHTNService::GetStaticTickIntervalDescription() const
{
	TStringBuilder<2048> StringBuilder;

	if (TickIntervalRandomDeviation > 0.0f)
	{
		StringBuilder << FString::Printf(TEXT("Tick every %.2fs..%.2fs"), 
			FMath::Max(0.0f, TickInterval - TickIntervalRandomDeviation), 
			TickInterval + TickIntervalRandomDeviation);
	}
	else
	{
		StringBuilder << FString::Printf(TEXT("Tick every %.2fs"), TickInterval);
	}	

	if (bUsePersistentTickCountdown)
	{
		StringBuilder << TEXT("\nUses persistent tick countdown");
	}

	return *StringBuilder;
}

float UHTNService::GetInterval() const
{
	if (TickIntervalRandomDeviation > 0.0f)
	{
		return FMath::FRandRange(
			FMath::Max(0.0f, TickInterval - TickIntervalRandomDeviation),
			TickInterval + TickIntervalRandomDeviation);
	}
	else
	{
		return TickInterval;
	}
}


void UHTNExtension_ServicePersistentTickCountdown::HTNStarted()
{
	Super::HTNStarted();

	ServiceToCountdown.Reset();
}

#if ENABLE_VISUAL_LOG
void UHTNExtension_ServicePersistentTickCountdown::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category(TEXT("HTN Service Persistent Tick Intervals"));

	for (const TPair<const UHTNService*, FIntervalCountdown>& Pair : ServiceToCountdown)
	{
		const UHTNService* const Service = Pair.Key;
		if (IsValid(Service))
		{
			Category.Add(
				*Service->GetShortDescription(),
				FString::Printf(TEXT("%.2fs until next tick"), Pair.Value.TimeLeft));
		}
	}

	Snapshot->Status.Add(MoveTemp(Category));
}
#endif

bool UHTNExtension_ServicePersistentTickCountdown::GetPersistentTickCountdown(FIntervalCountdown& OutCountdown, const UHTNService* Service) const
{
	if (const FIntervalCountdown* const Countdown = ServiceToCountdown.Find(Service))
	{
		OutCountdown = *Countdown;
		return true;
	}

	return false;
}

void UHTNExtension_ServicePersistentTickCountdown::SetPersistentTickCountdown(const UHTNService* Service, const FIntervalCountdown& Countdown)
{
	ServiceToCountdown.Add(Service, Countdown);
}
