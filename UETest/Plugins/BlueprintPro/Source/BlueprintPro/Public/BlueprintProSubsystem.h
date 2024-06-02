// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BlueprintProSubsystem.generated.h"


class UCoroutineHandle;

/**
 * 
 */
UCLASS()
class BLUEPRINTPRO_API UBlueprintProSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** @return an available UCoroutineHandle from the pool (possibly allocating a new Coroutine) */
	UCoroutineHandle* RequestCoroutine();

	/** Release a UCoroutineHandle returned by RequestCoroutine() back to the pool */
	void ReturnCoroutine(UCoroutineHandle* Coroutine);

	/** Release all Generated Coroutines back to the pool */
	void ReturnAllCoroutines();

	/** Release all GeneratedCoroutinees back to the pool and allow them to be garbage collected */
	void FreeAllCoroutines();
public:

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Implement this for deinitialization of instances of the system */
	virtual void Deinitialize() override;
protected:
	/** Coroutinees in the pool that are available */
	UPROPERTY()
	TArray<TObjectPtr<UCoroutineHandle>> CachedCoroutines;

	/** All Coroutinees the pool has allocated */
	UPROPERTY()
	TArray<TObjectPtr<UCoroutineHandle>> AllCreatedCoroutines;
};
