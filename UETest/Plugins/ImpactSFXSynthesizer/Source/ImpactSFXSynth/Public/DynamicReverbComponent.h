// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldCollision.h"
#include "DynamicReverbComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class IMPACTSFXSYNTH_API UDynamicReverbComponent : public UActorComponent
{
	GENERATED_BODY()

protected:

	/** This is passed directly to your graph through Modal Spatial interface.
	 * You can use this to enable/disable Modal HRTF node and modify your graph, instead of having to build separate graphs.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Spatial Settings")
	bool bEnableHRTF;

	/** Control the global head radius used in the ModalHRTF node.
	 * This should be set based on user's settings before BeginPlay is called to avoid audible pop noises.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Spatial Settings")
	float HeadRadius;
	
	/** The time tick for each trace frame.
	 * Because we use async trace, a new trace frame will be skipped if the previous one has not finished yet.
	 * Reduce this if your character movement is very fast. */
	UPROPERTY(EditDefaultsOnly, Category = "Tracing")
	float TraceTick;

	/** Reverb collision channel. You can use a custom channel here for better reverb control. */
	UPROPERTY(EditDefaultsOnly, Category = "Tracing")
	TEnumAsByte<ECollisionChannel> TraceChannel;

	/** The number of total traces per trace frame. */
	UPROPERTY(BlueprintReadOnly, Category = "Tracing")
	int32 NumTracePerFrame;

	/** For stability, the calculated results of each trace frame are stored and averaged together.	 
	 * Decrease this if your character movement is fast (racing cars, etc.).
	 * Or increase this if you want the result to be more stable. */
	UPROPERTY(EditDefaultsOnly, Category = "Tracing", meta = (ClampMin = "1", ClampMax = "32", UIMin = "1", UIMax = "32"))
	int32 NumInterFrame;

	/** If the distance between the current location and the last trace location is higher than this value, internal states are reset.	*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracing")
	float ResetDistance;
	
	/** The end distance of each trace. */
	UPROPERTY(EditDefaultsOnly, Category = "Tracing")
	FVector TraceEndVector;

	/** The height offset for the trace origin. */
	UPROPERTY(EditDefaultsOnly, Category = "Tracing")
	float HeightOffset;
	
	/** Draw a debug trace or not? Only used in Editor.*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracing Debug")
	bool bIsDrawTraceDebug;

	/** The time to keep each debug draw. Only used in Editor.*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracing Debug")
	float DebugDrawTime;

	/** If true, print calculated room stats. Only used in Editor.*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracing Debug")
	bool bLogRoomStats;
	
public:
	UDynamicReverbComponent();

	virtual void GetRoomStats(const FVector& AudioLocation,
							   float& OutRoomSize, float& OutOpenFactor, float& OutAbsorption, float& OutEchoVar) const;

	UFUNCTION(BlueprintCallable, Category = "Audio Spatialization")
	void EnableHRTF(const bool bEnable) { bEnableHRTF = bEnable; }

	UFUNCTION(BlueprintCallable, Category = "Audio Spatialization")
	void SetHeadRadius(const float InHeadRadius);
	
	bool GetEnableHRTF() const { return bEnableHRTF; }
	
protected:
	void ResetTempBuffers();
	virtual void PostInitProperties() override;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION()
	void OnStartTrace();

	void OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data);

	virtual void UpdateRoomStats();
	virtual float CalculateRoomStats();

protected:
	float CurrentRoomSize;

	float CurrentAbsorption;
	float CurrentEchoVar;
	float CurrentOpenRoomFactor;
	
	FVector CurrentLocation;
	FVector LastLocation;
	
	int32 NumDoneTraces;
	bool bIsRoomStatUpdated;

	TArray<FVector> Collisions;

	TArray<float> RoomSizeArray;
	TArray<float> EchoVarArray;
	TArray<int32> NumNoHitArray;
	int32 NumTotalMiss;
	int32 InterFrameIndex;
	
private:
	FTimerHandle TraceTimerHandle;
	FCollisionQueryParams CollisionQueryParams;
	FCollisionResponseParams CollisionResponseParams;
	FTraceHandle TraceHandle;
	FTraceDelegate TraceDelegate;
	float CurrentTraceYawnRange;
	float CurrentTracePitchRange;

	float OctMapResolution;
};