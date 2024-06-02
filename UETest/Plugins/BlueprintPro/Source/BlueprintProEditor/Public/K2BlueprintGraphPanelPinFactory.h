// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

class SGraphPin;

class BLUEPRINTPROEDITOR_API FK2BlueprintGraphPanelPinFactory : public FGraphPanelPinFactory
{
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;

//private:
	//
	//	bool IsSupportPinCategory(FName PinCategory);
};
