// Copyright YTSS 2022. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AsyncExecutionBlueprintTypes.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SyncExecOnce.generated.h"

/**
 * 
 */
UCLASS(Deprecated)
class THREADEXECUTIONBLUEPRINTNODE_API UDEPRECATED_USyncExecOnce : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/*Event flow executed in the main game thread*/
	UPROPERTY(BlueprintAssignable, DisplayName="Execution")
	FSimpleDynamicMuticastDelegate OnExecution;

	virtual void Activate() override;
	/**
	 * Executed once in the main game thread.
	 * Even if the node is in the main thread it will execute.
	 */
	UFUNCTION(BlueprintCallable, DisplayName="Create Game Thread Exec Once", Category="Thread|Once",
		meta=(BlueprintInternalUseOnly=true,DeprecatedFunction))
	static UDEPRECATED_USyncExecOnce* CreateSyncExecOnce();

protected:
	void Execution();
};
