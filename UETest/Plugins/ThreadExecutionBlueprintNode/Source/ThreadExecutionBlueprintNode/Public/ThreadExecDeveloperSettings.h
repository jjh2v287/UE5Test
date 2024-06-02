// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ThreadExecDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(Config="ThreadExecNodeSettings", DefaultConfig)
class THREADEXECUTIONBLUEPRINTNODE_API UThreadExecDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// UThreadExecDeveloperSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
	// 	:UDeveloperSettings(ObjectInitializer)
	// {}
	
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetSectionName() const override { return TEXT("ThreadExecNode"); }

#if WITH_EDITOR
	virtual FText GetSectionDescription() const override{return FText::FromString("");}
	virtual FText GetSectionText() const override{return FText::FromString("Thread Exec Node");}
#endif
	
	UFUNCTION(BlueprintPure,Category="Thread|Settings", DisplayName="Get Thread Exec Node Settings")
	static UThreadExecDeveloperSettings* Get() { return GetMutableDefault<UThreadExecDeveloperSettings>(); }

	// UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="ThreadExec")
	// float PendingThreadTaskDoneWhenEnding = 0.1f;
};
