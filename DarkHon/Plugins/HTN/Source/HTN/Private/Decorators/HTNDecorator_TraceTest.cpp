// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Decorators/HTNDecorator_TraceTest.h"

#include "AIController.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "VisualLogger/VisualLogger.h"

#include "WorldStateProxy.h"

UHTNDecorator_TraceTest::UHTNDecorator_TraceTest(const FObjectInitializer& Initializer) : Super(Initializer),
	TraceFromZOffset(0.0f),
	TraceToZOffset(0.0f),
	bUseComplexCollision(false),
	bIgnoreSelf(true),
	TraceShape(EEnvTraceShape::Line),
	TraceExtentX(0.0f),
	TraceExtentY(0.0f),
	TraceExtentZ(0.0f),
	DrawDebugType(EDrawDebugTrace::None),
	DebugColor(FLinearColor::Red),
	DebugHitColor(FLinearColor::Green),
	DebugDrawTime(5.0f)
{
	NodeName = TEXT("Trace Test");
	
	TraceFrom.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UHTNDecorator_TraceTest, TraceFrom), AActor::StaticClass());
	TraceFrom.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UHTNDecorator_TraceTest, TraceFrom));

	TraceTo.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UHTNDecorator_TraceTest, TraceTo), AActor::StaticClass());
	TraceTo.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UHTNDecorator_TraceTest, TraceTo));
}

void UHTNDecorator_TraceTest::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);

	UBlackboardData* const BBAsset = GetBlackboardAsset();
	if (ensure(BBAsset))
	{
		TraceFrom.ResolveSelectedKey(*BBAsset);
		TraceTo.ResolveSelectedKey(*BBAsset);
	}
}

FString UHTNDecorator_TraceTest::GetStaticDescription() const
{
	static const UEnum* const TraceShapeEnum = StaticEnum<EEnvTraceShape::Type>();
	static const UEnum* const CollisionChannelEnum = StaticEnum<ECollisionChannel>();
	
	return FString::Printf(TEXT("%s: %s trace\nfrom %s to %s\non %s\nMust %s"), 
		*Super::GetStaticDescription(),
		*TraceShapeEnum->GetDisplayNameTextByValue(TraceShape.GetValue()).ToString(),
		*TraceFrom.SelectedKeyName.ToString(), *TraceTo.SelectedKeyName.ToString(),
		*CollisionChannelEnum->GetDisplayNameTextByValue(CollisionChannel.GetValue()).ToString(),
		IsInversed() ? TEXT("not hit") : TEXT("hit")
	);
}

bool UHTNDecorator_TraceTest::CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const
{
	UWorldStateProxy* const WorldStateProxy = GetWorldStateProxy(OwnerComp, CheckType);
	if (!ensure(WorldStateProxy))
	{
		return false;
	}

	AActor* TraceFromActor = nullptr;
	const FVector TraceFromRawPosition = WorldStateProxy->GetLocation(TraceFrom, &TraceFromActor);
	if (!FAISystem::IsValidLocation(TraceFromRawPosition))
	{
		return false;
	}

	AActor* TraceToActor = nullptr;
	const FVector TraceToRawPosition = WorldStateProxy->GetLocation(TraceTo, &TraceToActor);
	if (!FAISystem::IsValidLocation(TraceToRawPosition))
	{
		return false;
	}

	const FVector StartLocation = TraceFromRawPosition + FVector(0.0f, 0.0f, TraceFromZOffset);
	const FVector EndLocation = TraceToRawPosition + FVector(0.0f, 0.0f, TraceToZOffset);
	const ETraceTypeQuery TraceTypeQuery = UEngineTypes::ConvertToTraceType(CollisionChannel);
	const FVector Extent(TraceExtentX, TraceExtentY, TraceExtentZ);
	FillActorsToIgnoreBuffer(OwnerComp, TraceFromActor, TraceToActor);
	ON_SCOPE_EXIT { ActorsToIgnoreBuffer.Reset(); };

	FHitResult Hit;
	bool bHit = false;
	switch (TraceShape)
	{
	case EEnvTraceShape::Line:
		bHit = UKismetSystemLibrary::LineTraceSingle(&OwnerComp, StartLocation, EndLocation,
			TraceTypeQuery, bUseComplexCollision,
			ActorsToIgnoreBuffer,
			DrawDebugType, Hit, /*bIgnoreSelf=*/false, DebugColor, DebugHitColor, DebugDrawTime
		);
		break;

	case EEnvTraceShape::Box:
		bHit = UKismetSystemLibrary::BoxTraceSingle(&OwnerComp, StartLocation, EndLocation,
			Extent, (EndLocation - StartLocation).Rotation(),
			TraceTypeQuery, bUseComplexCollision,
			ActorsToIgnoreBuffer,
			DrawDebugType, Hit, /*bIgnoreSelf=*/false, DebugColor, DebugHitColor, DebugDrawTime
		);
		break;

	case EEnvTraceShape::Sphere:
		bHit = UKismetSystemLibrary::SphereTraceSingle(&OwnerComp, StartLocation, EndLocation,
			Extent.X,
			TraceTypeQuery, bUseComplexCollision,
			ActorsToIgnoreBuffer,
			DrawDebugType, Hit, /*bIgnoreSelf=*/false, DebugColor, DebugHitColor, DebugDrawTime
		);
		break;

	case EEnvTraceShape::Capsule:
		bHit = UKismetSystemLibrary::CapsuleTraceSingle(&OwnerComp, StartLocation, EndLocation,
			Extent.X, Extent.Z,
			TraceTypeQuery, bUseComplexCollision,
			ActorsToIgnoreBuffer,
			DrawDebugType, Hit, /*bIgnoreSelf=*/false, DebugColor, DebugHitColor, DebugDrawTime
		);
		break;

	default:
		break;
	}

	UE_VLOG_ARROW(OwnerComp.GetOwner(), LogHTN, Verbose, 
		StartLocation, EndLocation, 
		bHit ? DebugHitColor.ToFColor(/*sRGB=*/true) : DebugColor.ToFColor(/*sRGB=*/true), TEXT("HTNDecorator_TraceTest")
	);
	return bHit;
}

void UHTNDecorator_TraceTest::FillActorsToIgnoreBuffer(UHTNComponent& OwnerComp, AActor* TraceFromActor, AActor* TraceToActor) const
{
	ActorsToIgnoreBuffer.Reset();
	if (bIgnoreSelf)
	{
		ActorsToIgnoreBuffer.Add(OwnerComp.GetOwner());
		if (const AAIController* const AIController = OwnerComp.GetAIOwner())
		{
			if (APawn* const Pawn = AIController->GetPawn())
			{
				ActorsToIgnoreBuffer.AddUnique(Pawn);
			}
		}
	}

	if (TraceFromActor)
	{
		ActorsToIgnoreBuffer.AddUnique(TraceFromActor);
	}

	if (TraceToActor)
	{
		ActorsToIgnoreBuffer.AddUnique(TraceToActor);
	}
}
