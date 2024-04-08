#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "UKTickIntervalFocusComponent.h"
#include "UKTickIntervalTargetComponent.h"
#include "UKTickIntervalSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FUKTargetTickIntervalInfo
{
	GENERATED_USTRUCT_BODY()
	
public:
	UPROPERTY()
	AActor* Owner;
	UPROPERTY()
	int32 Index = INDEX_NONE;
	UPROPERTY()
	int32 TickIntervalLayer = INDEX_NONE;
	UPROPERTY()
	float TickIntervalTime = 0.0f;
	UPROPERTY()
	FVector ActorLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FUKFocusTickIntervalInfo
{
	GENERATED_USTRUCT_BODY()
	
public:
	UPROPERTY()
	AActor* Owner;
	UPROPERTY()
	int32 Index = INDEX_NONE;
	UPROPERTY()
	FVector ActorLocation = FVector::ZeroVector;
	UPROPERTY()
	TArray<FUKTickIntervalSettings> TickIntervalSettings;
};

/**
 * 
 */
UCLASS()
class UETEST_API UKTickIntervalSubsystem
	: public UGameInstanceSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()
private:
	static inline UKTickIntervalSubsystem* Instance = nullptr;
	
public:
	static UKTickIntervalSubsystem* Get() { return Instance; }
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UUKObjectTickWorldSubsystem, STATGROUP_Tickables); }
	virtual void Tick(float DeltaTime) override;

	void RegisterFocus(UUKTickIntervalFocusComponent* Focus);
	void UnregisterFocus(UUKTickIntervalFocusComponent* Focus);
	void RegisterTarget(UUKTickIntervalTargetComponent* Target);
	void UnregisterTarget(UUKTickIntervalTargetComponent* Target);

	void UpdateTargetActorTick(float DeltaTime);
	const TTuple<int32, float, bool> GetTimeAndEnableToDistance(const TArray<FUKTickIntervalSettings>& TickIntervalSettings, const float Distance);
public:
	UFUNCTION(BlueprintPure)
	FORCEINLINE int32 GetTargetObjectNum()
	{
		return Targets.Num();
	}
	
private :
	UPROPERTY(Transient)
	TArray<UUKTickIntervalFocusComponent*> Focuses;

	UPROPERTY(Transient)
	TArray<UUKTickIntervalTargetComponent*> Targets;

	TArray<FUKFocusTickIntervalInfo> FocusTickIntervalInfos;
	TArray<FUKTargetTickIntervalInfo> TargetTickIntervalInfos;
	class TickIntervalThread* TickIntervalThreadRef;
};

#pragma region TickIntervalThread
class TickIntervalThread : public FRunnable
{
public:
	TickIntervalThread();
	virtual ~TickIntervalThread() override;
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	
	bool IsThreadPaused() const;
	void EnsureCompletion();
	void PauseThread();
	void ContinueThread();

	const TArray<FUKTargetTickIntervalInfo> GetActorData(TArray<FUKFocusTickIntervalInfo> FocusInfos, TArray<FUKTargetTickIntervalInfo> TargetInfos);

private:
	const TTuple<int32, float, bool> GetTimeAndEnableToDistance(const TArray<FUKTickIntervalSettings>& TickIntervalSettings, const float Distance);
	
private:
	FRunnableThread* Thread;
	FCriticalSection m_mutex;
	FEvent* m_semaphore;

	FThreadSafeBool m_Kill;
	FThreadSafeBool m_Pause;

	float threadSleepTime = 0.01f;
	float threadDeltaTime = 0.0f;

	TArray<FUKFocusTickIntervalInfo> FocusTickIntervalInfos;
	TArray<FUKTargetTickIntervalInfo> TargetTickIntervalInfos;
	TArray<FUKTargetTickIntervalInfo> TickIntervalInfoCluster;
};
#pragma endregion TickIntervalThread