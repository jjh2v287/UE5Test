// Copyright YTSS 2022. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DEPRECATED_Mutex.h"
#include "ThreadAsyncExecLoopUnsafely.h"
#include "ThreadAsyncExecTick.h"
#include "ThreadAsyncExecLibrary.generated.h"

/**
 * 
 */
UCLASS()
class THREADEXECUTIONBLUEPRINTNODE_API UThreadAsyncExecLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Utilities Begin

	/*Get the ID of the thread executing the node*/
	UFUNCTION(BlueprintPure, Category="Thread")
	static int32 GetCurrentThreadID();

	/*Get the thread name of the thread executing the node*/
	UFUNCTION(BlueprintPure, Category="Thread")
	static FName GetThreadName();

	UFUNCTION(BlueprintCallable,Category="Thread|Once")
	static void GameThreadExecOnce(const FSimpleDynamicDelegate& GameThreadExec);

	/*Set the thread name of the thread executing the node*/
	UFUNCTION(BlueprintCallable, Category="Thread")
	static bool SetThreadName(FName ThreadName);

	/*Whether the thread executing the node is a game thread*/
	UFUNCTION(BlueprintPure, Category="Thread")
	static bool IsGameThread();

	/*Whether the thread executing the node is a game thread*/
	UFUNCTION(BlueprintCallable, DisplayName="Is Game Thread", Category="Thread",
		meta=(ExpandBoolAsExecs=bIsInGameThread))
	static void ExecIsGameThread(bool& bIsInGameThread);

	/*Thread waiting. Not valid when used in a game thread*/
	UFUNCTION(BlueprintCallable, Category="Thread",
		meta=(CompactNodeTitle="THREAD WAIT", Keywords="Sleep", DeprecatedFunction))
	static void ThreadWait(float Seconds = 2.0f);

	// Utilities End

public:
	// Mutex Begin
	// Mutex End
};
