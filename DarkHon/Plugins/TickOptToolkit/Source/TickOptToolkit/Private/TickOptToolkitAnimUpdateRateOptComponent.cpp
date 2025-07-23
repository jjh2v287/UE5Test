// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitAnimUpdateRateOptComponent.h"
#include "TickOptToolkit.h"
#include "TickOptToolkitSettings.h"
#include "Rob.h"
#include "GameFramework/Actor.h"
#include "Components/SkinnedMeshComponent.h"

ROB_DEFINE_FUN(USkinnedMeshComponent, RefreshUpdateRateParams, void);

UTickOptToolkitAnimUpdateRateOptComponent::UTickOptToolkitAnimUpdateRateOptComponent()
	: Super()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTickOptToolkitAnimUpdateRateOptComponent::BeginPlay()
{
	Super::BeginPlay();

	const FConsoleVariableDelegate CVarChangeDelegate = FConsoleVariableDelegate::CreateUObject(this, &UTickOptToolkitAnimUpdateRateOptComponent::OnScalabilityCVarChanged);
	OnOptimizationLevelHandle = CVarTickOptToolkitUROOptimizationLevel->OnChangedDelegate().Add(CVarChangeDelegate);
	OnScreenSizeScaleHandle = CVarTickOptToolkitUROScreenSizeScale->OnChangedDelegate().Add(CVarChangeDelegate);

	if (const AActor* Owner = GetOwner())
	{
		for (UActorComponent* Component : Owner->GetComponents())
		{
			if (USkinnedMeshComponent* SkinnedMeshComponent = Cast<USkinnedMeshComponent>(Component))
			{
				RegisterDynamicSkinnedMeshComponent(SkinnedMeshComponent);
			}
		}
	}
}

void UTickOptToolkitAnimUpdateRateOptComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	CVarTickOptToolkitUROOptimizationLevel->OnChangedDelegate().Remove(OnOptimizationLevelHandle);
	CVarTickOptToolkitUROScreenSizeScale->OnChangedDelegate().Remove(OnScreenSizeScaleHandle);

	RegisteredSkinnedMeshComponents.Reset();
}

#if WITH_EDITOR
void UTickOptToolkitAnimUpdateRateOptComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static FName OptimizationLevelsName = GET_MEMBER_NAME_CHECKED(UTickOptToolkitAnimUpdateRateOptComponent, OptimizationLevels);
	
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const EPropertyChangeType::Type ChangeType = PropertyChangedEvent.ChangeType;

	if (PropertyName == OptimizationLevelsName && ChangeType == EPropertyChangeType::ArrayAdd)
	{
		const int32 Index = PropertyChangedEvent.GetArrayIndex(OptimizationLevelsName.ToString());
		if (OptimizationLevels.IsValidIndex(Index))
		{
			FTickOptToolkitUROOptimizationLevel& Level = OptimizationLevels[Index];
			Level.FramesSkippedScreenSizeThresholds = FramesSkippedScreenSizeThresholds;
			Level.LODToFramesSkipped = LODToFramesSkipped;
			Level.NonRenderedFramesSkipped = NonRenderedFramesSkipped;
			Level.MaxFramesSkippedForInterpolation = MaxFramesSkippedForInterpolation;
		}
	}
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UTickOptToolkitAnimUpdateRateOptComponent::RegisterDynamicSkinnedMeshComponent(USkinnedMeshComponent* SkinnedMeshComponent)
{
	if (RegisteredSkinnedMeshComponents.Find(SkinnedMeshComponent) != INDEX_NONE)
	{
		return;
	}
	RegisteredSkinnedMeshComponents.Add(SkinnedMeshComponent);
	
	SkinnedMeshComponent->OnAnimUpdateRateParamsCreated = FOnAnimUpdateRateParamsCreated::CreateUObject(this, &UTickOptToolkitAnimUpdateRateOptComponent::OnAnimUpdateRateParamsCreated);
	if (SkinnedMeshComponent->IsRegistered())
	{
		(SkinnedMeshComponent->*RobAccess(USkinnedMeshComponent, RefreshUpdateRateParams))();
		if (bForceEnableAnimUpdateRateOptimizations)
		{
			SkinnedMeshComponent->bEnableUpdateRateOptimizations = true;
		}
	}
}

void UTickOptToolkitAnimUpdateRateOptComponent::SetAnimUpdateRateOptimizationsMode(ETickOptToolkitAnimUROMode InAnimUpdateRateOptimizationsMode)
{
	if (!HasBegunPlay())
	{
		AnimUpdateRateOptimizationsMode = InAnimUpdateRateOptimizationsMode;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'AnimUpdateRateOptimizationsMode' property after BeginPlay!"));
	}
}

void UTickOptToolkitAnimUpdateRateOptComponent::SetNonRenderedFramesSkipped(int InNonRenderedFramesSkipped)
{
	if (!HasBegunPlay())
	{
		NonRenderedFramesSkipped = InNonRenderedFramesSkipped;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'NonRenderedFramesSkipped' property after BeginPlay!"));
	}
}

void UTickOptToolkitAnimUpdateRateOptComponent::SetMaxFramesSkippedForInterpolation(int InMaxFramesSkippedForInterpolation)
{
	if (!HasBegunPlay())
	{
		MaxFramesSkippedForInterpolation = InMaxFramesSkippedForInterpolation;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'MaxFramesSkippedForInterpolation' property after BeginPlay!"));
	}
}

void UTickOptToolkitAnimUpdateRateOptComponent::SetFramesSkippedScreenSizeThresholds(const TArray<float>& InFramesSkippedScreenSizeThresholds)
{
	if (!HasBegunPlay())
	{
		FramesSkippedScreenSizeThresholds = InFramesSkippedScreenSizeThresholds;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'FramesSkippedScreenSizeThresholds' property after BeginPlay!"));
	}
}

void UTickOptToolkitAnimUpdateRateOptComponent::SetLODToFramesSkipped(const TArray<int>& InLODToFramesSkipped)
{
	if (!HasBegunPlay())
	{
		LODToFramesSkipped = InLODToFramesSkipped;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'LODToFramesSkipped' property after BeginPlay!"));
	}
}

void UTickOptToolkitAnimUpdateRateOptComponent::SetForceEnableAnimUpdateRateOptimizations(bool bInForceEnableAnimUpdateRateOptimizations)
{
	if (!HasBegunPlay())
	{
		bForceEnableAnimUpdateRateOptimizations = bInForceEnableAnimUpdateRateOptimizations;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'ForceEnableAnimUpdateRateOptimizations' property after BeginPlay!"));
	}
}

void UTickOptToolkitAnimUpdateRateOptComponent::SetDisableScreenSizeScale(bool bInDisableScreenSizeScale)
{
	if (!HasBegunPlay())
	{
		bDisableScreenSizeScale = bInDisableScreenSizeScale;
	}
	else
	{
		UE_LOG(LogTickOptToolkit, Warning, TEXT("Cannot set 'bDisableScreenSizeScale' property after BeginPlay!"));
	}
}

void UTickOptToolkitAnimUpdateRateOptComponent::OnAnimUpdateRateParamsCreated(FAnimUpdateRateParameters* AnimUpdateRateParameters)
{
	const int32 OptimizationLevel = FMath::Clamp(CVarTickOptToolkitUROOptimizationLevel.GetValueOnGameThread(), 0, OptimizationLevels.Num());
#define TOT_URO_OPT_LEVEL_GET(Property) (OptimizationLevel == 0 ? Property : OptimizationLevels[OptimizationLevel - 1].Property)

	AnimUpdateRateParameters->BaseNonRenderedUpdateRate = TOT_URO_OPT_LEVEL_GET(NonRenderedFramesSkipped) + 1;
	AnimUpdateRateParameters->MaxEvalRateForInterpolation = TOT_URO_OPT_LEVEL_GET(MaxFramesSkippedForInterpolation) + 1;
	
	if (AnimUpdateRateOptimizationsMode == ETickOptToolkitAnimUROMode::ScreenSize)
	{
		AnimUpdateRateParameters->bShouldUseLodMap = false;

		const float ScreenSizeScale = bDisableScreenSizeScale ? 1.0f : CVarTickOptToolkitUROScreenSizeScale.GetValueOnGameThread();
		const TArray<float>& ScreenSizes = TOT_URO_OPT_LEVEL_GET(FramesSkippedScreenSizeThresholds);
		AnimUpdateRateParameters->BaseVisibleDistanceFactorThesholds.Empty(ScreenSizes.Num());
		for (const float ScreenSize : ScreenSizes)
		{
			AnimUpdateRateParameters->BaseVisibleDistanceFactorThesholds.Add(FMath::Square(ScreenSize * ScreenSizeScale));
		}
	}
	else
	{
		AnimUpdateRateParameters->bShouldUseLodMap = true;
		AnimUpdateRateParameters->bShouldUseMinLod = AnimUpdateRateOptimizationsMode == ETickOptToolkitAnimUROMode::MinLOD;

		const TArray<int>& LODs = TOT_URO_OPT_LEVEL_GET(LODToFramesSkipped);
		AnimUpdateRateParameters->LODToFrameSkipMap.Empty(LODs.Num());
		for (int LodI = 0; LodI < LODs.Num(); ++LodI)
		{
			AnimUpdateRateParameters->LODToFrameSkipMap.Add(LodI, LODs[LodI]);
		}
	}
#undef TOT_URO_OPT_LEVEL_GET
}

void UTickOptToolkitAnimUpdateRateOptComponent::OnScalabilityCVarChanged(IConsoleVariable*)
{
	for (USkinnedMeshComponent* SkinnedMeshComponent : RegisteredSkinnedMeshComponents)
	{
		if (IsValid(SkinnedMeshComponent) && SkinnedMeshComponent->IsRegistered())
		{
			(SkinnedMeshComponent->*RobAccess(USkinnedMeshComponent, RefreshUpdateRateParams))();
		}
	}
}
