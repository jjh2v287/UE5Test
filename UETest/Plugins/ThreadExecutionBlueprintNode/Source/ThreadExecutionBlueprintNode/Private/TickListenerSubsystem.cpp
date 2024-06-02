// Fill out your copyright notice in the Description page of Project Settings.


#include "TickListenerSubsystem.h"

#include "ThreadAsyncExecTick.h"
#include "Engine.h"
#include "Engine/World.h"

FName FTickFunctionByGroup::DiagnosticContext(bool bDetailed)
{
	if (bDetailed)
	{
		// Format is "ActorNativeClass/ActorClass"
		FString ContextString = FString::Printf(TEXT("%s/%s"), *GetParentNativeClass(Target->GetClass())->GetName(),
		                                        *Target->GetClass()->GetName());
		return FName(*ContextString);
	}
	else
	{
		return GetParentNativeClass(Target->GetClass())->GetFName();
	}
}

FString FTickFunctionByGroup::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[TickThreadNode]");
}

void FTickFunctionByGroup::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread,
                                       const FGraphEventRef& MyCompletionGraphEvent)
{
	if (Target && IsValidChecked(Target) && !Target->IsUnreachable())
	{
		FScopeCycleCounterUObject Scope(Target);
		Target->ExecuteTick(DeltaTime, TickType, TickGroupEnumOperator::ToThreadTickTiming(TickGroup.GetValue()));
	}
}


void UTickListenerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TickFunctionByGroups.Emplace(new FTickFunctionByGroup(this, TG_PrePhysics));
	TickFunctionByGroups.Emplace(new FTickFunctionByGroup(this, TG_DuringPhysics));
	TickFunctionByGroups.Emplace(new FTickFunctionByGroup(this, TG_PostPhysics));
	TickFunctionByGroups.Emplace(new FTickFunctionByGroup(this, TG_PostUpdateWork));

	ThreadNodeSubsystem = GEngine->GetEngineSubsystem<UThreadNodeSubsystem>();

	WorldPreActorTickHandle = FWorldDelegates::OnWorldPreActorTick.AddUObject
		(this, &UTickListenerSubsystem::WorldPreActorTick);
	WorldPostActorTickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject
		(this, &UTickListenerSubsystem::WorldPostActorTick);
}

void UTickListenerSubsystem::Deinitialize()
{
	Super::Deinitialize();
	FWorldDelegates::OnWorldPreActorTick.Remove(WorldPreActorTickHandle);
	FWorldDelegates::OnWorldPreActorTick.Remove(WorldPostActorTickHandle);
}

void UTickListenerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	for (auto const& TickFunctionByGroup : TickFunctionByGroups)
	{
		TickFunctionByGroup->bCanEverTick = true;
		TickFunctionByGroup->SetTickFunctionEnable(true);
		TickFunctionByGroup->RegisterTickFunction(GetWorld()->PersistentLevel);
	}
}

void UTickListenerSubsystem::WorldPreActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	ExecuteTick(DeltaSeconds, TickType, EThreadTickTiming::PreActorTick);
}

void UTickListenerSubsystem::WorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	ExecuteTick(DeltaSeconds, TickType, EThreadTickTiming::PostActorTick);
}

void UTickListenerSubsystem::ExecuteTick
(
	float DeltaTime,
	ELevelTick TickType,
	EThreadTickTiming CurrentTickTiming
)
{
	if (TickType != LEVELTICK_ViewportsOnly && GetWorld())
	{
		{
			// Process once exec
			auto Onces = ThreadNodeSubsystem->GetAllThreadExecOnces();
			for (auto Once : Onces)
			{
				if (!IsValidChecked(Once))
				{
					continue;
				}
				if (!(TickType != LEVELTICK_PauseTick || TickType == LEVELTICK_PauseTick && Once->
					IsExecuteWhenPaused()))
				{
					// Determine execute when pause
					continue;
				}
				if (Once->TimingPair.BeginIn == CurrentTickTiming)
				{
					Once->BeginExec(DeltaTime, TickType, CurrentTickTiming);
				}
				else if (Once->TimingPair.EndBefore == CurrentTickTiming)
				{
					Once->EndExec(DeltaTime, TickType, CurrentTickTiming);
				}
			}
		}
		{
			// Process ticks exec
			auto Ticks = ThreadNodeSubsystem->GetAllThreadExecTicks();
			for (auto Tick : Ticks)
			{
				if (!IsValidChecked(Tick) && !Tick->IsTickEnabled())
				{
					continue;
				}
				if (!(TickType != LEVELTICK_PauseTick || TickType == LEVELTICK_PauseTick && Tick->
					IsTickableWhenPaused()))
				{
					// Determine to tick when pause
					continue;
				}
				if (!Tick->IsTicking() && Tick->GetTimingPair().BeginIn == CurrentTickTiming)
				{
					Tick->BeginExec(DeltaTime, TickType, CurrentTickTiming);
				}
				else if (Tick->IsTicking() && Tick->GetTimingPair().EndBefore == CurrentTickTiming)
				{
					Tick->EndExec(DeltaTime, TickType, CurrentTickTiming);
				}
			}
		}
	}
}
