// Copyright YTSS 2022. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AsyncExecutionBlueprintTypes.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ThreadAsyncExecBase.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class THREADEXECUTIONBLUEPRINTNODE_API UThreadAsyncExecBase : public UBlueprintAsyncActionBase,
                                                              public FThreadAsyncExecInfo
{
	GENERATED_BODY()

public:
	UThreadAsyncExecBase() { ExecMask = EThreadAsyncExecTag::DoExecution; }

	virtual void Activate() override;
	virtual void BeginDestroy() override;

	virtual void Execution()PURE_VIRTUAL(UThreadAsyncExecBase::Execution)
	virtual void Completed();

protected:
	virtual void BeginDestroyButFutureNotReady();
	virtual void CompletedExecInGameThread();
};
