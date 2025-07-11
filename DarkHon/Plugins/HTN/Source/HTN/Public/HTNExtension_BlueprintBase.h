// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNExtension.h"
#include "HTNExtension_BlueprintBase.generated.h"

// The Blueprint base class for HTN extensions.
UCLASS(Abstract, Blueprintable)
class HTN_API UHTNExtension_BlueprintBase : public UHTNExtension
{
	GENERATED_BODY()

public:
	UHTNExtension_BlueprintBase();
	virtual void Initialize() override;
	virtual void HTNStarted() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Cleanup() override;

protected:
	uint8 bOnInitializeBPImplemented : 1;
	uint8 bOnHTNStartedBPImplemented : 1;
	uint8 bOnTickBPImplemented : 1;
	uint8 bOnCleanupBPImplemented : 1;

	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void OnInitialize();

	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void OnHTNStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void OnTick(float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void OnCleanup();
};