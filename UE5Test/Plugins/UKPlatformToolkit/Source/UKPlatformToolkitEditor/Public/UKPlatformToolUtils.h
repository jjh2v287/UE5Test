// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UKPlatformToolUtils.generated.h"

USTRUCT(BlueprintType)
struct FUKSelectActorPlatformInfo
{
	GENERATED_USTRUCT_BODY()
	
public:
	UPROPERTY()
	int32 PlatformActorNum = 0;
	UPROPERTY()
	int32 PlatformActorGroupNum = 0;
};

/**
 * 
 */
UCLASS(transient)
class UKPLATFORMTOOLKITEDITOR_API UUKPlatformToolUtils : public UObject
{
	GENERATED_BODY()
	UUKPlatformToolUtils();
private:
	static inline UUKPlatformToolUtils* Instance = nullptr;
	
public:
	static UUKPlatformToolUtils* Get() { return Instance; }

	FUKSelectActorPlatformInfo GetSelectPlatformGroupInfo();

	void PlatformGroup(bool IsBox);
	void PlatformUnGroup();
};
