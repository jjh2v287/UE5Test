// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ThreadNodeSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "TickListenerSubsystem.generated.h"

class UTickListenerSubsystem;

USTRUCT()
struct FTickFunctionByGroup : public FTickFunction
{
	GENERATED_BODY()

	explicit FTickFunctionByGroup
	(
		const TObjectPtr<::UTickListenerSubsystem>& Target = nullptr,
		ETickingGroup TickGroup = TG_PrePhysics,
		ETickingGroup EndTickGroup = TG_PrePhysics
	)
		: FTickFunction(),
		  Target(Target)
	{
		this->TickGroup = TickGroup;
		this->EndTickGroup = EndTickGroup;
		bTickEvenWhenPaused=true;
		bAllowTickOnDedicatedServer=true;
	}

	TObjectPtr<UTickListenerSubsystem> Target;

	virtual FName DiagnosticContext(bool bDetailed) override;
	virtual FString DiagnosticMessage() override;
	virtual void ExecuteTick
	(
		float DeltaTime,
		ELevelTick TickType,
		ENamedThreads::Type CurrentThread,
		const FGraphEventRef& MyCompletionGraphEvent
	) override;
};

template <>
struct TStructOpsTypeTraits<FTickFunctionByGroup> : public TStructOpsTypeTraitsBase2<FTickFunctionByGroup>
{
	enum
	{
		WithCopy = false,
		WithPureVirtual = true
	};
};


/**
 * 
 */
UCLASS()
class THREADEXECUTIONBLUEPRINTNODE_API UTickListenerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:
	FDelegateHandle WorldPreActorTickHandle;
	FDelegateHandle WorldPostActorTickHandle;
	void WorldPreActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);
	void WorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);

	TArray<TUniquePtr<FTickFunctionByGroup>> TickFunctionByGroups;

	UPROPERTY()
	TObjectPtr<UThreadNodeSubsystem> ThreadNodeSubsystem;

public:
	virtual void ExecuteTick
	(
		float DeltaTime,
		ELevelTick TickType,
		EThreadTickTiming CurrentTickTiming
	);
};
