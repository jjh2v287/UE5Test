// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeSchema.h"

#include "ZKStateTreeSchemaBase.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta=(DisplayName="ZK StateTreeSchema"))
class ZKGAME_API UZKStateTreeSchemaBase : public UStateTreeSchema
{
	GENERATED_BODY()
public:

	UZKStateTreeSchemaBase();

protected:

	// UStateTreeSchema interface.
	virtual bool IsStructAllowed(const UScriptStruct* InScriptStruct) const override;
	virtual bool IsClassAllowed(const UClass* InScriptStruct) const override;
	virtual bool IsExternalItemAllowed(const UStruct& InStruct) const override;
	virtual TConstArrayView<FStateTreeExternalDataDesc> GetContextDataDescs() const override { return ContextDataDescs; }

private:

	UPROPERTY()
	TArray<FStateTreeExternalDataDesc> ContextDataDescs;
};
