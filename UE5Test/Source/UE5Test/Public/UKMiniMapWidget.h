// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UKMapCaptureActor.h"
#include "UKMiniMapWidget.generated.h"

class UImage;

USTRUCT(BlueprintType)
struct FUKUICaptureMapInfos
{
	GENERATED_BODY()
public:
	FUKCaptureMapData CaptureMapDatas;
	FVector BoxExtent = FVector::Zero();
	float MiniMapRate = 1.0f;
};

USTRUCT(BlueprintType)
struct FUKUIEnclosingBoxInfos
{
	GENERATED_BODY()
public:
	TMap<FName, FUKUICaptureMapInfos> UICaptureMapInfos;
};

USTRUCT(BlueprintType)
struct FUKUIActiveImage
{
	GENERATED_BODY()
public:
	TWeakObjectPtr<UImage> Image;
	FUKUICaptureMapInfos CaptureMapDatas;
};

/**
 * 
 */
UCLASS()
class UE5TEST_API UUKMiniMapWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Minimap|Grid")
	void OnTileLoaderTick();

	void DataLoad();
	void LoadTile();
	void UpdateTile();
	void UnLoadTile();
	float GetMiniMapSize();
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = "Minimap|Binds", meta = (BindWidget))
	class USizeBox* SizeBox;
	
	UPROPERTY(BlueprintReadOnly, Category = "Minimap|Binds", meta = (BindWidget))
	class UCanvasPanel* MinimapCanvas;

	UPROPERTY(BlueprintReadWrite, Category = "Minimap|Core")
	float MiniMapUISize = 500.0f;
	
	UPROPERTY(BlueprintReadWrite, Category = "Minimap|Core")
	float MiniMapUIScale = 1.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Minimap|Grid")
	FTimerHandle TileLoaderTimerHandle;

	UPROPERTY(BlueprintReadWrite, Category = "Minimap|Core")
	float TileLoaderTick = 1.0f / 30.f;

	float MaxOrthoWidth = 0.0f;
	FString CurrentEnclosingBoxName;
	FUKCaptureMapInfos* CaptureMapInfos = nullptr;
	TMap<FName, FUKUIActiveImage> ActiveTileWidgets;
	TMap<FName, FUKUICaptureMapInfos> UICaptureMapInfos;
};
