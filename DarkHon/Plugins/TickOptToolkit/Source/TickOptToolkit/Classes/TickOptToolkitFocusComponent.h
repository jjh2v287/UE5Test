// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TickOptToolkitFocusComponent.generated.h"

class UTickOptToolkitSubsystem;

UENUM(meta = (Bitflags))
enum class ETickOptToolkitFocusLayer : uint8
{
	Layer1,
	Layer2,
	Layer3,
	Layer4,
	Layer5,
	Layer6,
	Layer7,
	Layer8,
	Layer9,
	Layer10,
	Layer11,
	Layer12,
	Layer13,
	Layer14,
	Layer15,
	Layer16,

	Count UMETA(Hidden)
};

UCLASS(ClassGroup=(TickOptToolkit), meta=(BlueprintSpawnableComponent),
	HideCategories=(Rendering, Physics, Collision, LOD, Activation, Cooking, ComponentReplication))
class TICKOPTTOOLKIT_API UTickOptToolkitFocusComponent : public USceneComponent
{
	GENERATED_BODY()

	friend class UTickOptToolkitSubsystem;

	UPROPERTY(Transient)
	UTickOptToolkitSubsystem* Subsystem;

	int FocusIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintGetter="GetFocusLayer", BlueprintSetter="SetFocusLayer", Category="TickOptToolkit")
	ETickOptToolkitFocusLayer FocusLayer = ETickOptToolkitFocusLayer::Layer1;

public:
	UTickOptToolkitFocusComponent();
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnRegister() override;
	virtual void OnUnregister() override;

public:
	/** Should this focus be tracked by the Tick Optimization Toolkit Subsystem.
	 *  Useful in multiplayer environment. By default will be tracked:
	 *		- on a server always,
	 *		- on a client if it's a component of a locally owned actor.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="TickOptToolkit")
	bool ShouldTrack();

	UFUNCTION(BlueprintGetter, Category="TickOptToolkit")
	ETickOptToolkitFocusLayer GetFocusLayer() const { return FocusLayer; }
	
	UFUNCTION(BlueprintSetter, Category="TickOptToolkit")
	void SetFocusLayer(ETickOptToolkitFocusLayer InFocusLayer)
	{
		FocusLayer = InFocusLayer;
	}
	
	virtual bool ShouldTrack_Implementation();
};
