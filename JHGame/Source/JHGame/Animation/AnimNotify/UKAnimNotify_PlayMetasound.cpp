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

		// Is UnregisterGraph -> RegisterGraphWithFrontend
		MetasoundSource->UnregisterGraphWithFrontend();
		MetasoundSource->InitResources();

		const int32 RandIndex = FMath::RandRange(0, SoundWaveAssets.Num() - 1);
		USoundWave* SendWave =  Cast<USoundWave>(SoundWaveAssets[RandIndex].LoadSynchronous());
		if (SendWave == nullptr)
		{
			return;
		}

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
		if (UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAttached(MetasoundSource, MeshComp, AttachName, Location, AttachType, true, VolumeMultiplier, PitchMultiplier))
		{
			const float Gain = SendWave->Volume;
			const float Duration = SendWave->Duration;
			const FString SoundName = SendWave->GetName();
			
			TArray<FAudioParameter> Params = AudioParams;
			Params.Emplace(FAudioParameter(SoundWaveParamName, SendWave));
			Params.Emplace(FAudioParameter(TEXT("FinalGains"), Gain));
			Params.Emplace(FAudioParameter(TEXT("Duration"), Duration));
			Params.Emplace(FAudioParameter(TEXT("WaveName"), SoundName));
			AudioComponent->SetParameters(MoveTemp(Params));
			AudioComponent->Play();
		}
	}
}
