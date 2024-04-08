// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKMinimapUIWidget.h"
#include <Kismet/KismetMathLibrary.h>
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
// #include "UKEventManager.h"
// #include "Actors/UKMapHelper.h"
#include <Engine/AssetManager.h>
#include "UKMapCaptureActor.h"
#include "UKMiniMapWidget.h"
#include "MathUtil.h"
#include "AssetRegistry/AssetData.h"
#include "Components/CanvasPanelSlot.h"
#include "Camera/CameraComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
// #include "GameFramework/UKPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKMinimapUIWidget)

void UUKMinimapUIWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	SetMapSize(FVector2D(4096, 4096));
}

void UUKMinimapUIWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdatePlayer();
	// UpdateMapRotation();
	// UpdateMapPosition();

	UpdateMapCaptureDataLoad();
	LoadGridImage();
	UpdateGridImageExtent();
	UpdateGridImageRotation();
	UpdateGridImageLocation();
	UnLoadGridImage();
}

// void UUKMinimapUIWidget::Register()
// {
// 	PlayerCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
// 	
// 	check(PlayerCharacter);
// 	
// 	UpdateInfo();
// 	MapCaptureDataLoad();
// 	return;
// 	
// 	EventHandler = UKEventManager::CreateHandler();
//
// 	FindWorldHelpers();
//
// 	UpdatePlayer();
// 	UpdateMapPosition();
// }

void UUKMinimapUIWidget::UpdateInfo()
{
	// TODO : 현재 날짜(년/월/일)와 날씨를 알려준다. 업데이트 주기도 설정해야함.
	DateText->SetText(FText::FromString(*FDateTime::Now().ToString(TEXT("%Y/%m/%d"))));
}

void UUKMinimapUIWidget::SetMapSize(FVector2D NewSize)
{
	MapSize = NewSize;
	
	//WorldMapSizeBox->SetWidthOverride(MapSize.X);
	//WorldMapSizeBox->SetHeightOverride(MapSize.Y);
}

void UUKMinimapUIWidget::UpdatePlayer() const
{
	// const float PlayerAngle = PlayerCharacter->GetActorForwardVector().RotateAngleAxis(PlayerForwardAngle, FVector::ZAxisVector).ToOrientationRotator().Yaw;
	// PlayerImage->SetRenderTransformAngle(PlayerAngle);

	const FString LocationStr = FString::Format(TEXT("{0} {1} {2}"), {
		FMath::CeilToInt32(PlayerCharacter->GetActorLocation().X), FMath::CeilToInt32(PlayerCharacter->GetActorLocation().Y * -1.0f), FMath::CeilToInt32(PlayerCharacter->GetActorLocation().Z) });
	LocationText->SetText(FText::FromString(LocationStr));
}

void UUKMinimapUIWidget::UpdateMapRotation()
{
	const float CameraAngle = (UGameplayStatics::GetPlayerController(GetWorld(), 0)->PlayerCameraManager->GetCameraRotation().Yaw + PlayerForwardAngle) * -1.0f;
	MapCanvas->SetRenderTransformAngle(CameraAngle);
}

void UUKMinimapUIWidget::UpdateMapPosition()
{
	const float MapMoveRangeX = (MapSize.X - MinimapSize.X) / 2;
	const float MapMoveRangeY = (MapSize.Y - MinimapSize.Y) / 2;

	/*const FVector2D CurrentMapPos = WorldMapSizeBox->GetRenderTransform().Translation;
	const FVector2D ClampedMapPos = FVector2D(FMath::Clamp(CurrentMapPos.X, -MapMoveRangeX, MapMoveRangeX),
											  FMath::Clamp(CurrentMapPos.Y, -MapMoveRangeY, MapMoveRangeY));

	WorldMapSizeBox->SetRenderTranslation(ClampedMapPos);*/
}

FVector2D UUKMinimapUIWidget::WorldLocationToMapPosition(const FVector2D& GameWorldLocation) const
{
	const FVector2D Location = ((GameWorldLocation - GameWorldCenter) / GameWorldSize) * MapSize;
	
	return Location;
}

void UUKMinimapUIWidget::FindWorldHelpers()
{
	// const TObjectPtr<AUKMapHelper> MapHelperActor = Cast<AUKMapHelper>(UGameplayStatics::GetActorOfClass(GetWorld(), AUKMapHelper::StaticClass()));
	// if (IsValid(MapHelperActor))
	// {
	// 	WorldNorthWestLocation = MapHelperActor->GetWorldNorthWestLocation();
	// 	WorldSouthEastLocation = MapHelperActor->GetWorldSouthEastLocation();
	//
	// 	GameWorldSize = (WorldNorthWestLocation - WorldSouthEastLocation).GetAbs();	
	// 	GameWorldCenter = (WorldSouthEastLocation + WorldNorthWestLocation) / 2;
	//
	// 	//MapHelperActor->Destroy();
	// }
}

// void UUKMinimapUIWidget::Unregister()
// {
// 	
// }

void UUKMinimapUIWidget::MapCaptureDataLoad()
{
	MiniMapGridSize = FMathf::Max(RootSizeBox->GetWidthOverride(), RootSizeBox->GetHeightOverride());
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(MapDatAassetPath);

	if (AssetData.IsValid())
	{
		UUKCaptureMapInfoDataAsset* Data = Cast<UUKCaptureMapInfoDataAsset> (AssetData.GetAsset());
		FString MapName = GetWorld()->GetMapName();
		if(Data)
		{
			CaptureMapInfos = Data->CaptureMapInfos.Find(MapName);
		}

		if(!CaptureMapInfos)
		{
			MapName = GetWorld()->GetName();
			CaptureMapInfos = Data->CaptureMapInfos.Find(MapName);
			MaxOrthoWidth = Data->MaxOrthoWidth;
		}
	}

	UpdateMapCaptureDataLoad();
}
void UUKMinimapUIWidget::UpdateMapCaptureDataLoad()
{
	if(!CaptureMapInfos)
	{
		return;
	}

	FVector UISizeBox = FVector(RootSizeBox->GetWidthOverride() * 2, RootSizeBox->GetHeightOverride() * 2, 0.0f);
	FTransform GridInfoTransform = FTransform();
	TMap<FName, FUKCaptureMapData> CaptureMapDatas;
	
	for (auto Element : CaptureMapInfos->EnclosingBoxDatas)
	{
		GridInfoTransform.SetLocation(Element.Value.BoxLocation);

		bool isInBox = UKismetMathLibrary::IsPointInBoxWithTransform(PlayerCharacter->GetActorLocation(), GridInfoTransform, Element.Value.BoxSize + UISizeBox);
		bool isEnclosingBox = EnclosingBox.Contains(Element.Key);
		if(isInBox && !isEnclosingBox)
		{
			CaptureMapDatas = MoveTemp(Element.Value.CaptureMapDatas);
			EnclosingBox.Emplace(Element.Key, true);
		}
		else if(!isInBox && isEnclosingBox)
		{
			EnclosingBox.Remove(Element.Key);
			for (const auto& [Name, CaptureMapData] : Element.Value.CaptureMapDatas)
			{
				if(UICaptureMapInfos.Contains(Name))
				{
					UICaptureMapInfos.Remove(Name);
				}

				if(ActiveGridImage.Contains(Name))
				{
					ActiveGridImage.Find(Name)->Get()->RemoveFromParent();
					ActiveGridImage.Remove(Name);
				}
			}
		}
	}

	for (const auto& [Name, CaptureMapData] : CaptureMapDatas)
	{
		FUKUICaptureMapInfos Info;
		Info.CaptureMapDatas = CaptureMapData;
		Info.MiniMapRate = GetMiniMapGridSize() / Info.CaptureMapDatas.OrthoWidth;
		const float ParentWidgetHalfSize = GetMiniMapGridSize() * 0.5f;
		const float Reciprocal = 1.0f / Info.MiniMapRate;
		Info.BoxExtent.X = Reciprocal * ParentWidgetHalfSize;
		Info.BoxExtent.Y = Info.BoxExtent.X;
		Info.BoxExtent *= MaxOrthoWidth / Info.CaptureMapDatas.OrthoWidth;
		
		UICaptureMapInfos.Emplace(Name, Info);
	}
}

void UUKMinimapUIWidget::LoadGridImage()
{
	const FVector PlayerCharacterLocation = PlayerCharacter->GetActorLocation();
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	static FVector2d Alignment = FVector2D(0.5f,0.5f);
	static  FAnchors ImgAnchors = FAnchors(0.5f);
	
	for (auto GridInfo : UICaptureMapInfos)
	{
		const FString TextureToLoad = GridInfo.Value.CaptureMapDatas.Path + GridInfo.Value.CaptureMapDatas.Name + TEXT(".") + GridInfo.Value.CaptureMapDatas.Name;
		const FVector GridInfoLocation = GridInfo.Value.CaptureMapDatas.RootLocation + GridInfo.Value.CaptureMapDatas.RelativeLocation;
		FTransform GridInfoTransform = FTransform();
		GridInfoTransform.SetLocation(GridInfoLocation);

		const bool bIsActiveTile = ActiveGridImage.Contains(GridInfo.Key);
		const bool bIsPointInBox = UKismetMathLibrary::IsPointInBoxWithTransform(PlayerCharacterLocation, GridInfoTransform, GridInfo.Value.CaptureMapDatas.BoxSize + GridInfo.Value.BoxExtent);
		if(!bIsActiveTile && bIsPointInBox)
		{
			// Load
			UImage* CurrentTileWidget = NewObject<UImage>(MapImageCanvas);
			GridInfo.Value.MiniMapRate = GetMiniMapGridSize() / GridInfo.Value.CaptureMapDatas.OrthoWidth;
			CurrentTileWidget->SetVisibility(ESlateVisibility::Visible);
			MapImageCanvas->AddChildToCanvas(CurrentTileWidget);
			
			const FSoftObjectPath SoftTexture = FSoftObjectPath(TextureToLoad);
			Streamable.RequestAsyncLoad(SoftTexture,
				[CurrentTileWidget, SoftTexture]() {
					if(SoftTexture.ResolveObject() != nullptr)
					{
						CurrentTileWidget->SetBrushFromTexture(CastChecked<UTexture2D>(SoftTexture.ResolveObject()));
					}
			});

			const float OrthoWidthRate = GridInfo.Value.CaptureMapDatas.OrthoWidth / MaxOrthoWidth;
			const float WidgetSize = GetMiniMapGridSize() * OrthoWidthRate;
			const FVector2d ImgWidgetSize = FVector2D(WidgetSize, WidgetSize);
			FVector Translation = (PlayerCharacterLocation - (GridInfo.Value.CaptureMapDatas.RootLocation + GridInfo.Value.CaptureMapDatas.RelativeLocation)) * GridInfo.Value.MiniMapRate;
			
			UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(CurrentTileWidget->Slot.Get());
			PanelSlot->SetSize(ImgWidgetSize);
			PanelSlot->SetAnchors(ImgAnchors);
			PanelSlot->SetAlignment(Alignment);

			Translation *= OrthoWidthRate;
			const float tempY = Translation.Y;
			Translation.Y = Translation.X;
			Translation.X = tempY * -1.0f;
			CurrentTileWidget->SetRenderTranslation(FVector2d(Translation.X, Translation.Y));
			
			ActiveGridImage.Emplace(GridInfo.Key, CurrentTileWidget);
		}
	}
}

void UUKMinimapUIWidget::UpdateGridImageExtent()
{
	// Calculate the border according to the UI size. For additional border size when loading and unloading.
	for (auto& Element : UICaptureMapInfos)
	{
		Element.Value.MiniMapRate = GetMiniMapGridSize() / Element.Value.CaptureMapDatas.OrthoWidth;
		const float ParentWidgetHalfSize = GetMiniMapGridSize() * 0.5f;
		const float Reciprocal = 1.0f / Element.Value.MiniMapRate;
		Element.Value.BoxExtent.X = Reciprocal * ParentWidgetHalfSize;
		Element.Value.BoxExtent.Y = Element.Value.BoxExtent.X;
		Element.Value.BoxExtent *= MaxOrthoWidth / Element.Value.CaptureMapDatas.OrthoWidth;
	}
}

void UUKMinimapUIWidget::UpdateGridImageRotation()
{
	const UCameraComponent* CameraComponent = PlayerCharacter->GetComponentByClass<UCameraComponent>();
	if(CameraComponent)
	{
		const FVector CameraForwardVector = CameraComponent->GetForwardVector();
		float CameraAngle = UKismetMathLibrary::Atan2(CameraForwardVector.X, CameraForwardVector.Y);
		CameraAngle = UKismetMathLibrary::RadiansToDegrees(CameraAngle);
		CameraAngle = UKismetMathLibrary::GenericPercent_FloatFloat((CameraAngle + 360.0f), 360.0f);
		CameraAngle = CameraAngle - 90.0f;
		MapCanvas->SetRenderTransformAngle(CameraAngle);
		
		const float PlayerAngle = PlayerCharacter->GetActorForwardVector().RotateAngleAxis(PlayerForwardAngle, FVector::ZAxisVector).ToOrientationRotator().Yaw;
		PlayerImage->SetRenderTransformAngle(CameraAngle + (PlayerAngle - 90.0f));
	}
}

void UUKMinimapUIWidget::UpdateGridImageLocation()
{
	FVector PlayerCharacterLocation = PlayerCharacter->GetActorLocation();
	for (auto Element : ActiveGridImage)
	{
		const auto GridInfo = UICaptureMapInfos.Find(Element.Key);
		const float OrthoWidthRate = GridInfo->CaptureMapDatas.OrthoWidth / MaxOrthoWidth;
		const float WidgetSize = GetMiniMapGridSize() * OrthoWidthRate;
		const FVector2d ImgWidgetSize = FVector2D(WidgetSize, WidgetSize);
		FVector Translation = (PlayerCharacterLocation - (GridInfo->CaptureMapDatas.RootLocation + GridInfo->CaptureMapDatas.RelativeLocation)) * GridInfo->MiniMapRate;

		Translation *= OrthoWidthRate;
		const float tempY = Translation.Y;
		Translation.Y = Translation.X;
		Translation.X = tempY * -1.0f;
		Element.Value->SetRenderTranslation(FVector2d(Translation.X, Translation.Y));

		UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Element.Value->Slot.Get());
		PanelSlot->SetSize(ImgWidgetSize);
	}
}

void UUKMinimapUIWidget::UnLoadGridImage()
{
	FVector TargetActorLocation = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	FVector GridInfoLocation = FVector::Zero();
	FTransform GridInfoTransform = FTransform();

	for (auto It = ActiveGridImage.CreateIterator(); It; ++It)
	{
		const auto GridInfo = UICaptureMapInfos.Find(It.Key());
		if(!GridInfo)
		{
			return;
		}
		
		GridInfoLocation = GridInfo->CaptureMapDatas.RootLocation + GridInfo->CaptureMapDatas.RelativeLocation;
		GridInfoTransform.SetLocation(GridInfoLocation);

		const bool bIsPointInBox = UKismetMathLibrary::IsPointInBoxWithTransform(TargetActorLocation, GridInfoTransform, GridInfo->CaptureMapDatas.BoxSize + GridInfo->BoxExtent);
		if(!bIsPointInBox)
		{
			It.Value()->RemoveFromParent();
			ActiveGridImage.Remove(It.Key());
		}
	}
}

float UUKMinimapUIWidget::GetMiniMapGridSize()
{
	return MiniMapGridSize * MiniMapGridScale;
}