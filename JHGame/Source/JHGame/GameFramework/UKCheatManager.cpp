// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKCheatManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JHGame/Common/UKCommonLog.h"
#include "WorldPartition/DataLayer/DataLayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKCheatManager)

void UUKCheatManager::RegisterCommand(const FString& Cmd, FOnUKCommand::FDelegate&& Delegate)
{
	CommandListeners.FindOrAdd(Cmd).Add(MoveTemp(Delegate));
}

void UUKCheatManager::BeginDestroy()
{
	Super::BeginDestroy();

	for (auto& [Key, Command] : CommandListeners)
	{
		Command.Clear();
	}
	CommandListeners.Empty();
}

void UUKCheatManager::Fly()
{
	if (const ACharacter* Character = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn()))
	{
		if (Character->GetCharacterMovement()->MovementMode == MOVE_Flying)
		{
			Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
			return;
		}
	}
	
	Super::Fly();
}

void UUKCheatManager::VisibleUI(bool bVisible)
{
}

void UUKCheatManager::CheatUI()
{
}

void UUKCheatManager::ToggleHUD()
{
}

void UUKCheatManager::TeleportTo(const FVector& Location)
{
}

void UUKCheatManager::Region(const FName& RegionName)
{
}

void UUKCheatManager::Layer(const FName& LayerName, int32 Activated)
{
}

void UUKCheatManager::ForceGC()
{
#if WITH_EDITOR
	GEngine->ForceGarbageCollection(true);
#endif
}

void UUKCheatManager::SetAAALayer(int bOnOffState) const
{
}

void UUKCheatManager::SetKuwahara(float NewValue) const
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APostProcessVolume::StaticClass(), FoundActors);
	for (AActor* FoundActor : FoundActors)
	{
		APostProcessVolume *AsPPV = Cast<APostProcessVolume>(FoundActor);
		TArray<FWeightedBlendable>* Blendables = &AsPPV->Settings.WeightedBlendables.Array;

		if (Blendables)
		{
			for (int idx = 0; idx < Blendables->Num(); idx++)
			{
				FWeightedBlendable* Blendable = &(*Blendables)[idx];
				if (Blendable->Object->GetName().Equals("MI-AnisoKuwahara_Inst_Tree"))
				{
					Blendable->Weight = NewValue;
					UE_LOG(LogTemp, Log, TEXT("Kuwahara Weight changed to %f"), Blendable->Weight);
					break;
				}
			}
		}
		
	}
}

void UUKCheatManager::SetPostProcessMaterialWeight(const FString& MaterialName, float NewValue) const
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APostProcessVolume::StaticClass(), FoundActors);
	for (AActor* FoundActor : FoundActors)
	{
		APostProcessVolume *AsPPV = Cast<APostProcessVolume>(FoundActor);
		TArray<FWeightedBlendable>* Blendables = &AsPPV->Settings.WeightedBlendables.Array;

		if (Blendables)
		{
			for (int idx = 0; idx < Blendables->Num(); idx++)
			{
				FWeightedBlendable* Blendable = &(*Blendables)[idx];
				if (Blendable->Object->GetName().Equals(MaterialName))
				{
					Blendable->Weight = NewValue;
					if (GetOuterAPlayerController())
					{
						GetOuterAPlayerController()->ClientMessage(FString::Printf(TEXT("[Blds] PPM %s's weight is now %f"), *MaterialName, NewValue));
					}
					break;
				}
			}
		}
	}
}

void UUKCheatManager::ToggleAFK() const
{
}

void UUKCheatManager::ToggleDebugAI() const
{
}

void UUKCheatManager::ToggleBGM()
{
}

void UUKCheatManager::ForceCrash()
{
	UK_LOG(Log, "ForceCrash");
}

void UUKCheatManager::Cheat(const FString& Argument ) const
{
	TArray<FString> Params;
	const FString Delim{ TEXT(" ") };
	Argument.ParseIntoArray(Params, *Delim);
	if (Params.IsEmpty())
	{
		return;
	}
	
	if (const auto It = CommandListeners.Find(Params[0].ToLower()))
	{
		Params.RemoveAt(0);
		It->Broadcast(Params);
	}
}

void UUKCheatManager::DebugGizmo()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	bDebugGizmo = !bDebugGizmo;
	if (bDebugGizmo)
	{
		DebugGizmoActors.Append(GetWorld()->GetCurrentLevel()->Actors);
	}
	else
	{
		DebugGizmoActors.Empty(DebugGizmoActors.Num());
	}
#endif
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
void UUKCheatManager::TickGizmoDebug()
{
	const TObjectPtr<UWorld> LocalWorld = GetWorld();
	for (const TWeakObjectPtr<AActor>& TargetActor : DebugGizmoActors)
	{
		if (!TargetActor.IsValid() || TargetActor->IsHidden())
		{
			continue;
		}
		
		const FVector DebugLocation = TargetActor->GetActorLocation();
		
		const FColor DebugColorX = FColor::Red;
		const FVector DebugVectorX = TargetActor->GetActorForwardVector() * 100.f;
		DrawDebugDirectionalArrow(LocalWorld, DebugLocation, DebugLocation + DebugVectorX,
			3.f, DebugColorX, false, -1.f, static_cast<uint8>('\000'), 3.f);

		const FColor DebugColorY = FColor::Green;
		const FVector DebugVectorY = TargetActor->GetActorRightVector() * 100.f;
		DrawDebugDirectionalArrow(LocalWorld, DebugLocation, DebugLocation + DebugVectorY,
			3.f, DebugColorY, false, -1.f, static_cast<uint8>('\000'), 3.f);

		const FColor DebugColorZ = FColor::Blue;
		const FVector DebugVectorZ = TargetActor->GetActorUpVector() * 100.f;
		DrawDebugDirectionalArrow(LocalWorld, DebugLocation, DebugLocation + DebugVectorZ,
			3.f, DebugColorZ, false, -1.f, static_cast<uint8>('\000'), 3.f);
	}
}
#endif

void ApplyGameplayTagCount(const TObjectPtr<APlayerController>& PlayerController, const FGameplayTag& BlockTag)
{
	const TObjectPtr<UAbilitySystemComponent> PlayerASC{ PlayerController ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerController->GetCharacter()) : nullptr };
	if (!IsValid(PlayerASC))
	{
		return;
	}
	
	const int32 Count{ PlayerASC->HasMatchingGameplayTag(BlockTag) ? 0 : 1 };
	PlayerASC->SetLooseGameplayTagCount(BlockTag, Count);
}

void UUKCheatManager::TakeDamage(float Value)
{
}

void UUKCheatManager::InfiniteHealth() const
{
}

void UUKCheatManager::InfiniteStamina() const
{
}

void UUKCheatManager::InfiniteAP() const
{
}

void UUKCheatManager::ToggleMonsterAI() const
{
}

void UUKCheatManager::ToggleAwakening() const
{
}

void UUKCheatManager::KillPlayer()
{
	TakeDamage(UE_MAX_FLT);
}

void UUKCheatManager::MarkerTeleportTo(const FName& MarkerName)
{
}

void UUKCheatManager::PlayLevelSequence(const FSoftObjectPath& LevelSequencePath)
{
}

void UUKCheatManager::ToggleMarkerImGUI()
{
}

void UUKCheatManager::RefreshFoliageActorManager()
{
}

void UUKCheatManager::SetUKLanguage(const FString& Language)
{
}

void UUKCheatManager::ChangePlayerHair(const FName& TypeName)
{
}