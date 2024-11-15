// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "UKCheatManager.generated.h"

enum class EUKUIState : uint8;
enum class EUKDroppedActionType : uint8;
class UGameplayEffect;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnUKCommand, const TArray<FString>&);

/**
 * 
 */
UCLASS()
class JHGAME_API UUKCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	void RegisterCommand(const FString& Cmd, FOnUKCommand::FDelegate&& Delegate);
	
private:
	TMap<FString, FOnUKCommand> CommandListeners;
	
public:
	virtual void BeginDestroy() override;

	virtual void Fly() override;
	
public:
	/////////////////////////////////////////////
	// UI System
	/////////////////////////////////////////////
	
	UFUNCTION(exec)
	void VisibleUI(bool bVisible);

	UFUNCTION(exec)
	void CheatUI();
	UFUNCTION(exec)
	void ToggleHUD();
	/////////////////////////////////////////////

public:
	// AFK 애니메이션 출력 여부를 토글합니다.
	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void ToggleAFK() const;

	// AI 디버깅을 토글합니다.
	UFUNCTION(Exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void ToggleDebugAI() const;

	UFUNCTION(Exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void ToggleBGM();
	
	UFUNCTION(Exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void ForceCrash();
	
	UFUNCTION(Exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void Cheat(const FString& Argument) const;

	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void TeleportTo(const FVector& Location);

	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void Region(const FName& RegionName);

	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void Layer(const FName& LayerName, int32 Activated);
	
	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
    void ForceGC();
	
	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void SetAAALayer(int bOnOffState) const;

	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void SetKuwahara(float NewValue) const;
	
	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void SetPostProcessMaterialWeight(const FString& MaterialName, float NewValue) const;

	/** Toggle gizmo visualize. */
	UFUNCTION(exec)
	void DebugGizmo();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
public:
	/** Do any trace debugging that is currently enabled */
	void TickGizmoDebug();

	uint32 bDebugGizmo:1;
	
	TArray<TWeakObjectPtr<AActor>> DebugGizmoActors;
#endif

	/* Cheats for attribute */

	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void TakeDamage(float Value);
	
	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void InfiniteHealth() const;

	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void InfiniteStamina() const;

	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void InfiniteAP() const;
	
	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void ToggleMonsterAI() const;

	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void ToggleAwakening() const;
	
public:
	/* 사망 처리 */
	
	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void KillPlayer();

public:
	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void MarkerTeleportTo(const FName& MarkerName);

public:
	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void PlayLevelSequence(const FSoftObjectPath& LevelSequencePath);

public:
	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void ToggleMarkerImGUI();

public:
	UFUNCTION(exec, BlueprintCallable, Category="UKGame Cheat Manager")
	void RefreshFoliageActorManager();
	
public:
	UFUNCTION(exec)
	void SetUKLanguage(const FString& Language);
	
	UFUNCTION(exec)
	void ChangePlayerHair(const FName& TypeName);
};