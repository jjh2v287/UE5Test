// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "GameFramework/Actor.h"
#include "UKStreamingSourceProvider.generated.h"

class UWorldPartitionStreamingSourceComponent;

UCLASS()
class DARKHON_API AUKStreamingSourceProvider : public AActor
{
	GENERATED_BODY()

public:
	explicit AUKStreamingSourceProvider(const FObjectInitializer& ObjectInitializer);

public:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UWorldPartitionStreamingSourceComponent* GetWorldPartitionStreamingSource() const
	{
		return WorldPartitionStreamingSource;
	}

	void Active()
	{
		WorldPartitionStreamingSource->EnableStreamingSource();
	}
	
	void Deactive()
	{
		WorldPartitionStreamingSource->DisableStreamingSource();
	}

	UPROPERTY(EditAnywhere, Category = "Shape Generator")
	FVector BoxExtent = FVector::Zero();

	UPROPERTY(EditAnywhere, Category = "Shape Generator")
	float MaxShapeRadius = 1000.0f;
	
#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Shape Generator")
	void ShapeGenerator();
#endif
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWorldPartitionStreamingSourceComponent* WorldPartitionStreamingSource = nullptr;
};