// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/AudioEngineSubsystem.h"
#include "UKAudioEngineSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UETEST_API UUKAudioEngineSubsystem : public UAudioEngineSubsystem
{
	GENERATED_BODY()
public: 
	virtual ~UUKAudioEngineSubsystem() override = default;

	//~ Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface

	//~ Begin UAudioEngineSubsystem interface
	virtual void Update() override;
	//~ End UAudioEngineSubsystem interface
};

namespace Audio
{
	namespace OcclusionInterface
	{
		const extern FName Name;

		namespace Inputs
		{
			const extern FName Occlusion;
		} // namespace Inputs

		Audio::FParameterInterfacePtr GetInterface();
	} // namespace OcclusionInterface
} // namespace Audio