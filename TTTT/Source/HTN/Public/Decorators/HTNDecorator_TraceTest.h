// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HTNDecorator_TraceTest.generated.h"

// The condition of this decorator passes based on a trace between two points (or actors) being blocked.
// (By default, must be blocked to pass).
// When tracing from/to actors, they're automatically excluded from the trace test.
UCLASS()
class HTN_API UHTNDecorator_TraceTest : public UHTNDecorator
{
	GENERATED_BODY()

public:
	UHTNDecorator_TraceTest(const FObjectInitializer& Initializer);
	virtual void InitializeFromAsset(class UHTN& Asset) override;
	virtual FString GetStaticDescription() const override;
	
	UPROPERTY(EditAnywhere, Category = Trace)
	FBlackboardKeySelector TraceFrom;

	UPROPERTY(EditAnywhere, Category = Trace)
	float TraceFromZOffset;

	UPROPERTY(EditAnywhere, Category = Trace)
	FBlackboardKeySelector TraceTo;

	UPROPERTY(EditAnywhere, Category = Trace)
	float TraceToZOffset;

	UPROPERTY(EditAnywhere, Category = Trace)
	TEnumAsByte<ECollisionChannel> CollisionChannel;

	UPROPERTY(EditAnywhere, Category = Trace)
	uint8 bUseComplexCollision : 1;

	UPROPERTY(EditAnywhere, Category = Trace)
	uint8 bIgnoreSelf : 1;

	UPROPERTY(EditAnywhere, Category = "Trace|Shape")
	TEnumAsByte<EEnvTraceShape::Type> TraceShape;

	// Parameter for tracing shapes instead of lines.
	UPROPERTY(EditAnywhere, Category = "Trace|Shape", meta = (UIMin = 0, ClampMin = 0, EditCondition = "TraceShape != EEnvTraceShape::Line"))
	float TraceExtentX;

	// Parameter for tracing shapes instead of lines.
	UPROPERTY(EditAnywhere, Category = "Trace|Shape", meta = (UIMin = 0, ClampMin = 0, EditCondition = "TraceShape == EEnvTraceShape::Box"))
	float TraceExtentY;

	// Parameter for tracing shapes instead of lines.
	UPROPERTY(EditAnywhere, Category = "Trace|Shape", meta = (UIMin = 0, ClampMin = 0, EditCondition = "TraceShape == EEnvTraceShape::Box || TraceShape == EEnvTraceShape::Capsule"))
	float TraceExtentZ;

	UPROPERTY(EditAnywhere, Category = "Trace|Debug")
	TEnumAsByte<EDrawDebugTrace::Type> DrawDebugType;

	// The color of the debug lines before the hit location (or the end of the trace if nothing was hit).
	UPROPERTY(EditAnywhere, Category = "Trace|Debug", meta = (EditCondition = "DrawDebugType != EDrawDebugTrace::None"))
	FLinearColor DebugColor;

	// The color of the debug lines after the hit location.
	UPROPERTY(EditAnywhere, Category = "Trace|Debug", meta = (EditCondition = "DrawDebugType != EDrawDebugTrace::None"))
	FLinearColor DebugHitColor;

	// Time in seconds to keep the debug lines on screen when DrawDebugType is set to ForDuration.
	UPROPERTY(EditAnywhere, Category = "Trace|Debug", meta = (EditCondition = "DrawDebugType == EDrawDebugTrace::ForDuration"))
	float DebugDrawTime;

protected:
	virtual bool CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const override;
	void FillActorsToIgnoreBuffer(UHTNComponent& OwnerComp, AActor* TraceFromActor, AActor* TraceToActor) const;

	UPROPERTY(Transient)
	mutable TArray<TObjectPtr<AActor>> ActorsToIgnoreBuffer;
};
