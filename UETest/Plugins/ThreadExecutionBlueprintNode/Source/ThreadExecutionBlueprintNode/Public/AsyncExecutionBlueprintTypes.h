// Copyright YTSS 2022. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Async/Async.h"
#include "Engine/EngineBaseTypes.h"
#include "AsyncExecutionBlueprintTypes.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSimpleDynamicMuticastDelegate);

DECLARE_DYNAMIC_DELEGATE(FSimpleDynamicDelegate);

// namespace EThreadAsyncExecTag
// {
// 	enum Type:uint8
// 	{
// 		DoExecution = 1,
// 		WithoutCompletion = 2
// 	};
// }

UENUM()
enum class EThreadAsyncExecTag:uint8
{
	DoExecution = 1,
	WithoutCompletion = 2
};

ENUM_CLASS_FLAGS(EThreadAsyncExecTag)


struct FThreadAsyncExecInfo
{
protected:
	// Async loop tag
	EThreadAsyncExecTag ExecMask = EThreadAsyncExecTag(0);
	// Async Future
	TFuture<void> Future;
	// Async thread type
	EAsyncExecution AsyncExecution = EAsyncExecution::TaskGraph;

	FName ThreadName;

public:
	void SetExecMask(EThreadAsyncExecTag NewTag) { ExecMask |= NewTag; }

	void ClearExecMask(EThreadAsyncExecTag OldTag) { ExecMask &= ~OldTag; }

	bool HasExecMask(EThreadAsyncExecTag Tag) const { return bool(ExecMask & Tag); }

	EThreadAsyncExecTag GetExecMask() const { return ExecMask; }

	FName GetThreadName() const { return ThreadName; }
};

UENUM(BlueprintType)
enum class EThreadTickTiming:uint8
{
	PreActorTick UMETA(DisplayName="Pre Actor Tick"),
	PrePhysics UMETA(DisplayName="Pre Physics"),
	DuringPhysics UMETA(DisplayName="During Physics"),
	PostPhysics UMETA(DisplayName="Post Physics"),
	PostUpdateWork UMETA(DisplayName="Post Update Work"),
	PostActorTick UMETA(DisplayName="Post Actor Tick"),
};

// Pair of EThreadTickTiming
USTRUCT(BlueprintType)
struct THREADEXECUTIONBLUEPRINTNODE_API FThreadExecTimingPair
{
	GENERATED_BODY()
	// Timing of start execution
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Thread")
	EThreadTickTiming BeginIn = EThreadTickTiming::PrePhysics;
	// Timing of end execution
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Thread")
	EThreadTickTiming EndBefore = EThreadTickTiming::PostUpdateWork;
};

namespace TickGroupEnumOperator
{
	inline EThreadTickTiming ToThreadTickTiming(const ETickingGroup& TickGroup)
	{
		switch (TickGroup)
		{
		case TG_PrePhysics:
			return EThreadTickTiming::PrePhysics;
		case TG_DuringPhysics:
			return EThreadTickTiming::DuringPhysics;
		case TG_PostPhysics:
			return EThreadTickTiming::PostPhysics;
		case TG_PostUpdateWork:
			return EThreadTickTiming::PostUpdateWork;
		default:
			return EThreadTickTiming::PreActorTick;
		}
	}

	inline ETickingGroup ToTickingGroup(const EThreadTickTiming& ThreadTickTiming)
	{
		switch (ThreadTickTiming)
		{
		case EThreadTickTiming::PreActorTick:
			return TG_PrePhysics;
		case EThreadTickTiming::PrePhysics:
			return TG_PrePhysics;
		case EThreadTickTiming::DuringPhysics:
			return TG_DuringPhysics;
		case EThreadTickTiming::PostPhysics:
			return TG_PostPhysics;
		case EThreadTickTiming::PostUpdateWork:
			return TG_PostUpdateWork;
		case EThreadTickTiming::PostActorTick:
			return TG_PostUpdateWork;
		default:
			return TG_PrePhysics;
		}
	}
}

UCLASS()
class THREADEXECUTIONBLUEPRINTNODE_API UAsyncExecutionBlueprintTypes : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Thread|Conversions", DisplayName="To ThreadTickTiming (TickingGroup)",
		meta=(CompactNodeTitle="->", Keywords="cast convert", BlueprintAutocast))
	static EThreadTickTiming Conv_TickingGroupToThreadTickTiming(const ETickingGroup& TickGroup)
	{
		return TickGroupEnumOperator::ToThreadTickTiming(TickGroup);
	}

	UFUNCTION(BlueprintPure, Category = "Thread|Conversions", DisplayName="To TickingGroup (ThreadTickTiming)",
		meta=(CompactNodeTitle="->", Keywords="cast convert", BlueprintAutocast))
	static ETickingGroup Conv_ThreadTickTimingToTickingGroup(const EThreadTickTiming& ThreadTickTiming)
	{
		return TickGroupEnumOperator::ToTickingGroup(ThreadTickTiming);
	}
};


// class DelegateName : public TBaseDynamicMulticastDelegate<FWeakObjectPtr, void>
// {
// public:
// 	DelegateName();
// 	explicit DelegateName(const TMulticastScriptDelegate<>& InMulticastScriptDelegate);
// 	void Broadcast() const { DelegateName_DelegateWrapper(FUNC_CONCAT(*this)); }
//
// private:
// 	static void DelegateName_DelegateWrapper(DelegateName);
// };
