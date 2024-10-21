// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAnimNotify_PlayMetasound.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundAttenuation.h"
#include "AudioDevice.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKAnimNotify_PlayMetasound)

UUKAnimNotify_PlayMetasound::UUKAnimNotify_PlayMetasound()
	: Super()
{
}

FString UUKAnimNotify_PlayMetasound::GetNotifyName_Implementation() const
{
	if (!SoundWaveAssets.IsEmpty() && SoundWaveAssets[0].IsValid())
	{
		return SoundWaveAssets[0]->GetName();
	}
	else
	{
		return Super::GetNotifyName_Implementation();
	}
}

void UUKAnimNotify_PlayMetasound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	const UWorld* World = MeshComp->GetWorld();
	if (World == nullptr)
	{
		return;
	}
	
#if WITH_EDITORONLY_DATA
	if (World->WorldType == EWorldType::EditorPreview)
	{
		if (!SoundWaveAssets.IsEmpty())
		{
			const int32 RandIndex = FMath::RandRange(0, SoundWaveAssets.Num() - 1);
			UGameplayStatics::PlaySound2D(World, SoundWaveAssets[RandIndex].LoadSynchronous(), VolumeMultiplier, PitchMultiplier);
		}
	}
	else
#endif
	{
		UMetaSoundSource* MetasoundSource = Cast<UMetaSoundSource>(MetaSoundAsset.LoadSynchronous());
		if (MetasoundSource == nullptr)
		{
			return;
		}

		MetasoundSource->UnregisterGraphWithFrontend();
		MetasoundSource->InitResources();

		if (USoundClass* SoundClassObject = SoundClassObjectAsset.LoadSynchronous())
		{
			MetasoundSource->SoundClassObject = SoundClassObject;
		}

		if (USoundAttenuation* AttenuationSettings = AttenuationSettingsAsset.LoadSynchronous())
		{
			MetasoundSource->AttenuationSettings = AttenuationSettings;
		}
		
		const EAttachLocation::Type AttachType = bFollow ? EAttachLocation::SnapToTarget : EAttachLocation::KeepWorldPosition;
		const FVector Location = bFollow ? FVector::ZeroVector : MeshComp->GetComponentLocation();
		if (UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAttached(MetasoundSource, MeshComp, AttachName, Location, AttachType, false, VolumeMultiplier, PitchMultiplier))
		{
			TArray<UObject*> SendWave;
			SendWave.Reserve(SoundWaveAssets.Num());
			for (int i = 0; i < SoundWaveAssets.Num(); i++)
			{
				if (!SoundWaveAssets[i].IsNull())
				{
					SendWave.Emplace(SoundWaveAssets[i].LoadSynchronous());
				}
			}

			TArray<float> Gains;
			Gains.Reserve(SendWave.Num());
			for (int i = 0; i < SendWave.Num(); i++)
			{
				if (USoundWave* WaveBase = Cast<USoundWave>(SendWave[i]))
				{
					const float Gain = WaveBase->Volume;
					Gains.Emplace(Gain);
				}
			}

			TArray<FAudioParameter> Params = AudioParams;
			Params.Emplace(FAudioParameter(SoundWaveParamName, SendWave));
			Params.Emplace(FAudioParameter(TEXT("FinalGains"), Gains));
			AudioComponent->SetParameters(MoveTemp(Params));
		}
	}
}
