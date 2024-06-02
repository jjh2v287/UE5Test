// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "DEPRECATED_Mutex.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Deprecated)
class THREADEXECUTIONBLUEPRINTNODE_API UDEPRECATED_Mutex : public UObject
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

protected:
	FCriticalSection Mutex;

public:
	UFUNCTION(BlueprintCallable, Category="Thread|Mutex", meta=(DeprecatedFunction))
	void Lock() { Mutex.Lock(); }

	UFUNCTION(BlueprintCallable, Category="Thread|Mutex", meta=(DeprecatedFunction))
	bool TryLock() { return Mutex.TryLock(); }

	UFUNCTION(BlueprintCallable, Category="Thread|Mutex", meta=(DeprecatedFunction))
	void UnLock() { Mutex.Unlock(); }
};
