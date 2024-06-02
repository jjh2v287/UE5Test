// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SGraphPinNameList.h"

/** Class Forward Declarations */
class UEdGraphPin;

class BLUEPRINTPROEDITOR_API SGraphPinArraySortPropertyName : public SGraphPinNameList
{
public:
	SLATE_BEGIN_ARGS(SGraphPinArraySortPropertyName) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj, const TArray<FString>& InNameList);

	SGraphPinArraySortPropertyName();
	virtual ~SGraphPinArraySortPropertyName();

protected:

	void RefreshNameList(const TArray<FString>& InNameList);

};
