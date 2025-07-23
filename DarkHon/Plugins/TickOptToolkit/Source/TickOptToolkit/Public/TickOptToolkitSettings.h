// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HAL/IConsoleManager.h"
#include "TickOptToolkitFocusComponent.h"
#include "TickOptToolkitSettings.generated.h"

extern TICKOPTTOOLKIT_API TAutoConsoleVariable<int32> CVarTickOptToolkitTickOptimizationLevel;
extern TICKOPTTOOLKIT_API TAutoConsoleVariable<float> CVarTickOptToolkitTickDistanceScale;

extern TICKOPTTOOLKIT_API TAutoConsoleVariable<int32> CVarTickOptToolkitUROOptimizationLevel;
extern TICKOPTTOOLKIT_API TAutoConsoleVariable<float> CVarTickOptToolkitUROScreenSizeScale;

UCLASS(Config=Engine, DefaultConfig)
class TICKOPTTOOLKIT_API UTickOptToolkitSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Config, Category="Settings")
	bool bLimitTargetVisualizations = true;
	
	UPROPERTY(EditAnywhere, Config, Category="Settings")
	bool bSupportFocusLayers = false;

	UPROPERTY(EditAnywhere, Config, Category="Settings")
	FName FocusLayerNameOverrides[(uint8)ETickOptToolkitFocusLayer::Count];

	UPROPERTY(EditAnywhere, Config, Category="Settings")
	int BalancingFramesNum = 60;
	
#if WITH_EDITOR
	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) override;

private:
	void UpdateFocusLayerNameOverrides() const;
#endif // WITH_EDITOR
};
