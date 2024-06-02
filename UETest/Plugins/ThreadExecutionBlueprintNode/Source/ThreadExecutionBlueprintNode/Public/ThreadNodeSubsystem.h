// Copyright YTSS 2022. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DEPRECATED_Mutex.h"
#include "ThreadAsyncExecLoopUnsafely.h"
#include "ThreadAsyncExecOnce.h"
#include "ThreadAsyncExecTick.h"
#include "Subsystems/EngineSubsystem.h"
#include "ThreadNodeSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class THREADEXECUTIONBLUEPRINTNODE_API UThreadNodeSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
	TSet<TObjectPtr<UThreadAsyncExecBase>> Nodes;

	FCriticalSection NodesMutex;

	void CheckPointers();

public:
	void AddThreadObject(UThreadAsyncExecBase* NewNode);
	void RemoveThreadObject(UThreadAsyncExecBase* OldNode);

	template <typename NodeType=UThreadAsyncExecBase>
	TSet<NodeType*> GetAllThreadExecNodesTemplate();

	UFUNCTION(BlueprintCallable, Category="Thread|Subsystem")
	TSet<UThreadAsyncExecBase*> GetAllThreadExecNodes();

	UFUNCTION(BlueprintCallable, Category="Thread|Subsystem")
	TSet<UThreadAsyncExecOnce*> GetAllThreadExecOnces();

	UFUNCTION(BlueprintCallable, Category="Thread|Subsystem")
	TSet<UThreadAsyncExecLoopUnsafely*> GetAllThreadExecLoops();

	UFUNCTION(BlueprintCallable, Category="Thread|Subsystem")
	TSet<UThreadAsyncExecTickBase*> GetAllThreadExecTicks();


	// Mutex Manager
protected:
	// UPROPERTY(meta=(DeprecatedProperty))
	// TSet<UDEPRECATED_Mutex*> Mutexes;

public:
	UFUNCTION(BlueprintCallable, Category="Thread|Subsystem", meta=(DeprecatedFunction))
	UDEPRECATED_Mutex* CreateNewMutex();

	UFUNCTION(BlueprintCallable, Category="Thread|Subsystem", meta=(DeprecatedFunction))
	TSet<UDEPRECATED_Mutex*> GetAllMutexes();

	UFUNCTION(BlueprintCallable, Category="Thread|Subsystem", meta=(DeprecatedFunction))
	void DestoryMutex(UDEPRECATED_Mutex* Mutex);

};


template <typename NodeType>
TSet<NodeType*> UThreadNodeSubsystem::GetAllThreadExecNodesTemplate()
{
	FScopeLock ScopeLock(&NodesMutex);
	CheckPointers();
	TSet<NodeType*> SpecificTypeOfNodes;
	for (auto Node : Nodes)
	{
		if (Node->IsA(NodeType::StaticClass()))
		{
			SpecificTypeOfNodes.Add(Cast<NodeType>(Node));
		}
	}
	return SpecificTypeOfNodes;
}
