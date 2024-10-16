// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MetasoundSource.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UKAnimNotify_PlayMetasound.generated.h"

class USoundClass;
class USoundAttenuation;

/**
 * 
 */
// UCLASS()
UCLASS(const, hidecategories=Object, collapsecategories, Config = Game, meta=(DisplayName="Play Metasound"))
class JHGAME_API UUKAnimNotify_PlayMetasound : public UAnimNotify
{
	GENERATED_BODY()

public:
	UUKAnimNotify_PlayMetasound();
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metasound")
	TArray<TSoftObjectPtr<USoundWave>> SoundWaveAssets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metasound")
	FName SoundWaveParamName = TEXT("WaveAssets");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metasound")
    TSoftObjectPtr<UMetaSoundSource> MetaSoundAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metasound")
	TSoftObjectPtr<USoundClass> SoundClassObjectAsset;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metasound")
	TSoftObjectPtr<USoundAttenuation> AttenuationSettingsAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metasound")
	TArray<FAudioParameter> AudioParams;
	
	// Volume Multiplier
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metasound")
	float VolumeMultiplier{1.0f};

	// Pitch Multiplier
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metasound")
	float PitchMultiplier{1.0f};
	
	// If this sound should follow its owner
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metasound")
	bool bFollow{false};

	// Socket or bone name to attach sound to
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metasound", meta=(EditCondition="bFollow", ExposeOnSpawn = true))
	FName AttachName{NAME_None};
};
