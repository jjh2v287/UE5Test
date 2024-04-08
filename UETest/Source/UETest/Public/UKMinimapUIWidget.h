// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UKMapCaptureActor.h"
// #include "Actors/UKMapCaptureActor.h"
// #include "UKUIWidget.h"
#include "UKMiniMapWidget.h"
#include "Blueprint/UserWidget.h"
#include "UKMinimapUIWidget.generated.h"

enum class EUKMarkerType : uint8;

struct UETEST_API FUKMapMarkerInfo;

class APlayerController;
class ACharacter;

class UCanvasPanel;
class UImage;
class UOverlay;
class USizeBox;
class UTextBlock;
class UUKEventHandler;
class UETEST_API UUKMapMarkerUIWidget;

// USTRUCT(BlueprintType)
// struct FUKUICaptureMapInfos
// {
// 	GENERATED_BODY()
// public:
// 	FUKCaptureMapData CaptureMapDatas;
// 	FVector BoxExtent = FVector::Zero();
// 	float MiniMapRate = 1.0f;
// };

/**
 * 
 */
UCLASS()
class UETEST_API UUKMinimapUIWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// virtual void Register() override;	
	// virtual void Unregister() override;

	void MapCaptureDataLoad();
	void UpdateMapCaptureDataLoad();
	void LoadGridImage();
	void UpdateGridImageExtent();
	void UpdateGridImageRotation();
	void UpdateGridImageLocation();
	void UnLoadGridImage();
	float GetMiniMapGridSize();

private:
	void UpdateInfo();
	void SetMapSize(FVector2D NewSize);

	void UpdatePlayer() const;
	void UpdateMapPosition();
	void UpdateMapRotation();

	FVector2D WorldLocationToMapPosition(const FVector2D& GameWorldLocation) const;

	// MarkerTestActor in world
	void FindWorldHelpers();

protected:
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<ACharacter> PlayerCharacter = nullptr;	
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;
	// UPROPERTY(Transient)
	// TObjectPtr<UUKEventHandler> EventHandler;

	/* BindWidgets */
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget))
	TObjectPtr<USizeBox> RootSizeBox = nullptr;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MapCanvas = nullptr;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MapImageCanvas = nullptr;
	// UPROPERTY(BlueprintReadOnly, Meta = (BindWidget))
	// TObjectPtr<UImage> MapImage = nullptr;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget))
	TObjectPtr<UCanvasPanel> PlayerCanvas = nullptr;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget))
	TObjectPtr<UImage> PlayerImage = nullptr;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget))
	TObjectPtr<UImage> CameraImage = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget))
	TObjectPtr<UCanvasPanel> InfoCanvas = nullptr;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget))
	TObjectPtr<UTextBlock> DateText = nullptr;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidgetOptional))
	TObjectPtr<UImage> CloudImage = nullptr;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidgetOptional))
	TObjectPtr<UImage> SunImage = nullptr;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidgetOptional))
	TObjectPtr<UImage> RainImage = nullptr;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidgetOptional))
	TObjectPtr<UImage> LightingImage = nullptr;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget))
	TObjectPtr<UTextBlock> LocationText = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UK|UI")
	TObjectPtr<UTexture2D> MapTexture = nullptr;

	UPROPERTY(BlueprintReadOnly, Meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> AreaNameOverlay;
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> TravelOverlay;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UK|UI")
	// TMap<EUKMarkerType, TSubclassOf<UUKMapMarkerUIWidget>> MarkerClassMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UK|UI")
	FVector2D MapSize = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Core")
	FString MapDatAassetPath = TEXT("/Game/MapCapture/MapCaptureDataAsset.MapCaptureDataAsset");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Core")
	float MiniMapGridScale = 1.0f;

	float MiniMapGridSize = 200.0f;
	float MaxOrthoWidth = 0.0f;
	TMap<FName, TWeakObjectPtr<UImage>> ActiveGridImage;
	TMap<FName, FUKUICaptureMapInfos> UICaptureMapInfos;
	TMap<FString, bool> EnclosingBox;
	FUKCaptureMapInfos* CaptureMapInfos = nullptr;
	
	/* GameWorldSize - NorthWest(LeftTop) */ 
	FVector2D WorldNorthWestLocation = FVector2D(-3000.f, -3000.f);
	/* GameWorldSize - SouthEast(RightBottom) */
	FVector2D WorldSouthEastLocation = FVector2D(3000.f, 3000.f);
	/* GameWorldSize - UnrealUnit */
	FVector2D GameWorldSize = FVector2D(6000.0f, 6000.0f);
	/* GameWorldSize - UnrealUnit */
	FVector2D GameWorldCenter = FVector2D(0.0f, 0.0f);

private:
	FVector2D MinimapSize = FVector2D::ZeroVector;
	/* Player - look North */
	const float PlayerForwardAngle = 90.0f;

	/* Marker */
	TMap<EUKMarkerType, TObjectPtr<UOverlay>> OverlayMap;
	TArray<UUKMapMarkerUIWidget*> AreaNameMarkerWidgets;
	TArray<UUKMapMarkerUIWidget*> TravelMarkerWidgets;
	TMap<EUKMarkerType, TArray<UUKMapMarkerUIWidget*>> MarkerWidgetMap;
};