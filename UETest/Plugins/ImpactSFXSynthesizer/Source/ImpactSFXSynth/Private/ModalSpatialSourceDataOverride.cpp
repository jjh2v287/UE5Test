// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ModalSpatialSourceDataOverride.h"
#include "ModalSpatialParameterInterface.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "DynamicReverbComponent.h"
#include "ImpactSFXSynthLog.h"

FModalSpatialSourceDataOverride::FModalSpatialSourceDataOverride()
{
}

void FModalSpatialSourceDataOverride::Initialize(const FAudioPluginInitializationParams InitializationParams)
{
    
}


void FModalSpatialSourceDataOverride::OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId,
                                                   USourceDataOverridePluginSourceSettingsBase* InSettings)
{
   
}


void FModalSpatialSourceDataOverride::OnReleaseSource(const uint32 SourceId)
{
   
}


void FModalSpatialSourceDataOverride::GetSourceDataOverrides(const uint32 SourceId, const FTransform& InListenerTransform, FWaveInstance* InOutWaveInstance)
{
    const Audio::FParameterInterfacePtr paInterface = ModalSpatialParameterInterface::GetInterface();
    
    if (InOutWaveInstance->ActiveSound->GetSound()->ImplementsParameterInterface(paInterface))
    {
        float EnableHRTF = 1.f;
        float RoomSize = 1.f;
        float Absorption = 2.f;
        float OpenRoomDecayScale = 10.f;
        float EchoVar = 0.01f;
        
        if(const UDynamicReverbComponent* DynamicReverbComponent = GetNearestDynamicReverbComponent(InOutWaveInstance))
        {
            DynamicReverbComponent->GetRoomStats(InOutWaveInstance->Location,
                                              RoomSize, OpenRoomDecayScale, Absorption, EchoVar);
            EnableHRTF = DynamicReverbComponent->GetEnableHRTF();
        }
        
        // Send the parameters to the MetaSound interface
        Audio::IParameterTransmitter* paramTransmitter = InOutWaveInstance->ActiveSound->GetTransmitter();
        if (paramTransmitter != nullptr)
        {
            TArray<FAudioParameter> paramsToUpdate;
            paramsToUpdate.Add({ModalSpatialParameterInterface::Inputs::EnableHRTF, EnableHRTF});
            paramsToUpdate.Add({ModalSpatialParameterInterface::Inputs::RoomSize, RoomSize});
            paramsToUpdate.Add({ModalSpatialParameterInterface::Inputs::Absorption, Absorption});
            paramsToUpdate.Add({ModalSpatialParameterInterface::Inputs::OpenRoomFactor, OpenRoomDecayScale});
            paramsToUpdate.Add({ModalSpatialParameterInterface::Inputs::EchoVar, EchoVar});
            
            paramTransmitter->SetParameters(MoveTemp(paramsToUpdate));
        }
    }
}

void FModalSpatialSourceDataOverride::OnAllSourcesProcessed()
{
   
}

UDynamicReverbComponent* FModalSpatialSourceDataOverride::GetNearestDynamicReverbComponent(const FWaveInstance* InOutWaveInstance)
{
    UDynamicReverbComponent* NearestReverbComponent = nullptr;
    if(const UWorld* World = InOutWaveInstance->ActiveSound->GetWorld())
    {
        float NearestDistance = -1.f;
        for(FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            if(const APlayerController* PlayerController = Iterator->Get())
            {
                if(const APawn* PlayerPawn = PlayerController->GetPawn())
                {
                    if(UDynamicReverbComponent* ReverbComponent = PlayerPawn->FindComponentByClass<UDynamicReverbComponent>())
                    {
                        const float SqrtDistance = FVector::DistSquared(PlayerPawn->GetActorLocation(), InOutWaveInstance->Location);
                        if(NearestDistance < 0.f || NearestDistance > SqrtDistance)
                        {
                            NearestDistance = SqrtDistance;
                            NearestReverbComponent = ReverbComponent;
                        }
                    }
                }
            }
        }
    }
    return NearestReverbComponent;
}
