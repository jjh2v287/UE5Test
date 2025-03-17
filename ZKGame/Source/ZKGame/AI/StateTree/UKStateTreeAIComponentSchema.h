// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeComponentSchema.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "UKStateTreeAIComponentSchema.generated.h"

class UBlackboardComponent;
/**
 * 
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta = (DisplayName = "UK StateTreeAIComponent", CommonSchema))
class UUKStateTreeAIComponentSchema : public UStateTreeComponentSchema
{
	GENERATED_BODY()
public:
	UUKStateTreeAIComponentSchema(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void PostLoad() override;

	virtual bool IsStructAllowed(const UScriptStruct* InScriptStruct) const override;

	static bool SetContextRequirements(UBrainComponent& BrainComponent, FStateTreeExecutionContext& Context, bool bLogErrors = false);

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

protected:
	/** AIController class the StateTree is expected to run on. Allows to bind to specific Actor class' properties. */
	UPROPERTY(EditAnywhere, Category = "Defaults", NoClear)
	TSubclassOf<AAIController> AIControllerClass = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Defaults", NoClear)
	TSubclassOf<UBlackboardComponent> BlackboardComponentClass = nullptr;
};
