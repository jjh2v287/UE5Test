// Fill out your copyright notice in the Description page of Project Settings.

#include "UKMiniMapWidget.h"
#include <Kismet/KismetMathLibrary.h>
#include <Components/Image.h>
#include <Blueprint/WidgetTree.h>
#include <Engine/AssetManager.h>
#include "Components/CanvasPanelSlot.h"
#include "UKMapCaptureActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"

void UUKMiniMapWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UUKMiniMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	DataLoad();
	GetWorld()->GetTimerManager().SetTimer(TileLoaderTimerHandle, this, &UUKMiniMapWidget::OnTileLoaderTick, TileLoaderTick, true);
}

void UUKMiniMapWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UUKMiniMapWidget::OnTileLoaderTick()
{
	if(CaptureMapInfos == nullptr)
	{
		return;
	}
	
	TMap<FName, FUKCaptureMapData> CaptureMapDatas;;
	FVector TargetActorLocation = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	for (auto Element : CaptureMapInfos->EnclosingBoxDatas)
	{
		FTransform GridInfoTransform = FTransform();
		GridInfoTransform.SetLocation(Element.Value.BoxLocation);
		
		bool isInBox = UKismetMathLibrary::IsPointInBoxWithTransform(TargetActorLocation, GridInfoTransform, Element.Value.BoxSize + FVector(SizeBox->GetWidthOverride() * 2, SizeBox->GetHeightOverride() * 2, 0.0f));
		if(isInBox && !CurrentEnclosingBoxName.Equals(Element.Key))
		{
			CurrentEnclosingBoxName = Element.Key;
		}
	}
	
	for (const auto& [Name, CaptureMapData] : CaptureMapDatas)
	{
		FUKUICaptureMapInfos Info;
		Info.CaptureMapDatas = CaptureMapData;

		if(MaxOrthoWidth < Info.CaptureMapDatas.OrthoWidth)
		{
			MaxOrthoWidth = Info.CaptureMapDatas.OrthoWidth;
		}
		
		Info.MiniMapRate = GetMiniMapSize() / Info.CaptureMapDatas.OrthoWidth;
		float ParentWidgetHalfSize = FMathf::Max(SizeBox->GetWidthOverride(), SizeBox->GetHeightOverride()) * 0.5f;
		float Reciprocal = 1.0f / Info.MiniMapRate;
		Info.BoxExtent.X = Reciprocal * ParentWidgetHalfSize;
		Info.BoxExtent.Y = Info.BoxExtent.X;
		Info.BoxExtent *= MaxOrthoWidth / Info.CaptureMapDatas.OrthoWidth;
		
		UICaptureMapInfos.Emplace(Name, Info);
	}
	
	LoadTile();
	UpdateTile();
	UnLoadTile();
}

void UUKMiniMapWidget::DataLoad()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FString Path = TEXT("/Game/MapCapture/MapCaptureDataAsset.MapCaptureDataAsset");
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(Path);

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
		}
	}

	if(!CaptureMapInfos)
		return;

	ActiveTileWidgets.Empty();
	UICaptureMapInfos.Empty();

	TMap<FName, FUKCaptureMapData> CaptureMapDatas;;
	FVector TargetActorLocation = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	for (auto Element : CaptureMapInfos->EnclosingBoxDatas)
	{
		FTransform GridInfoTransform = FTransform();
		GridInfoTransform.SetLocation(Element.Value.BoxLocation);
		
		bool isInBox = UKismetMathLibrary::IsPointInBoxWithTransform(TargetActorLocation, GridInfoTransform, Element.Value.BoxSize);
		if(isInBox)
		{
			CurrentEnclosingBoxName = Element.Key;
			CaptureMapDatas = MoveTemp(Element.Value.CaptureMapDatas);
		}
		
	}
	
	for (auto Element : CaptureMapDatas)
	{
		UICaptureMapInfos.Emplace(Element.Key);
		
		FUKUICaptureMapInfos Info;
		Info.CaptureMapDatas = Element.Value;

		if(MaxOrthoWidth < Info.CaptureMapDatas.OrthoWidth)
		{
			MaxOrthoWidth = Info.CaptureMapDatas.OrthoWidth;
		}
		
		Info.MiniMapRate = GetMiniMapSize() / Info.CaptureMapDatas.OrthoWidth;
		float ParentWidgetHalfSize = FMathf::Max(SizeBox->GetWidthOverride(), SizeBox->GetHeightOverride()) * 0.5f;
		float Reciprocal = 1.0f / Info.MiniMapRate;
		Info.BoxExtent.X = Reciprocal * ParentWidgetHalfSize;
		Info.BoxExtent.Y = Info.BoxExtent.X;
		Info.BoxExtent *= MaxOrthoWidth / Info.CaptureMapDatas.OrthoWidth;
		
		UICaptureMapInfos.Emplace(Element.Key, Info);
	}
}

void UUKMiniMapWidget::LoadTile()
{
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

	for (auto GridInfo : UICaptureMapInfos)
	{
		FVector GridInfoLocation = GridInfo.Value.CaptureMapDatas.RootLocation + GridInfo.Value.CaptureMapDatas.RelativeLocation;
		FTransform GridInfoTransform = FTransform();
		GridInfoTransform.SetLocation(GridInfoLocation);
		FVector TargetActorLocation = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
		if(!ActiveTileWidgets.Find(GridInfo.Key)
			&& UKismetMathLibrary::IsPointInBoxWithTransform(TargetActorLocation, GridInfoTransform, GridInfo.Value.CaptureMapDatas.BoxSize + GridInfo.Value.BoxExtent))
		{
			// Load
			FString TextureToLoad = GridInfo.Value.CaptureMapDatas.Path + GridInfo.Value.CaptureMapDatas.Name + TEXT(".") + GridInfo.Value.CaptureMapDatas.Name;
			
			UImage* CurrentTileWidget = NewObject<UImage>(MinimapCanvas);
			GridInfo.Value.MiniMapRate = GetMiniMapSize() / GridInfo.Value.CaptureMapDatas.OrthoWidth;
			CurrentTileWidget->SetVisibility(ESlateVisibility::Visible);
			MinimapCanvas->AddChildToCanvas(CurrentTileWidget);
			
			const FSoftObjectPath SoftTexture = FSoftObjectPath(TextureToLoad);
			Streamable.RequestAsyncLoad(SoftTexture,
				[CurrentTileWidget, SoftTexture]() {
					if(SoftTexture.ResolveObject() != nullptr)
						CurrentTileWidget->SetBrushFromTexture(CastChecked<UTexture2D>(SoftTexture.ResolveObject()));
			});

			UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(CurrentTileWidget->Slot.Get());
			PanelSlot->SetSize(FVector2D(GetMiniMapSize() * (GridInfo.Value.CaptureMapDatas.OrthoWidth / MaxOrthoWidth), GetMiniMapSize() * (GridInfo.Value.CaptureMapDatas.OrthoWidth / MaxOrthoWidth)));
			FAnchors Anchors;
			Anchors.Minimum = FVector2D(0.5f,0.5f);
			Anchors.Maximum = FVector2D(0.5f,0.5f);
			PanelSlot->SetAnchors(Anchors);
			PanelSlot->SetAlignment(FVector2D(0.5f,0.5f));

			FVector Translation = (TargetActorLocation - (GridInfo.Value.CaptureMapDatas.RootLocation + GridInfo.Value.CaptureMapDatas.RelativeLocation)) * GridInfo.Value.MiniMapRate;
			Translation *= (GridInfo.Value.CaptureMapDatas.OrthoWidth / MaxOrthoWidth);
			float tempY = Translation.Y;
			Translation.Y = Translation.X;
			Translation.X = tempY * -1.0f;
			CurrentTileWidget->SetRenderTranslation(FVector2d(Translation.X, Translation.Y));

			FUKUIActiveImage ActiveImage;
			ActiveImage.Image = CurrentTileWidget;
			ActiveImage.CaptureMapDatas = GridInfo.Value;
			ActiveTileWidgets.Add(GridInfo.Key, ActiveImage);
		}
	}
}

void UUKMiniMapWidget::UpdateTile()
{
	for (auto& Element : UICaptureMapInfos)
	{
		// UI 크기에 따른 경계를 계산한다 로드 언로드시 경계의 추가 사이즈를 위해서
		Element.Value.MiniMapRate = GetMiniMapSize() / Element.Value.CaptureMapDatas.OrthoWidth;
		float ParentWidgetHalfSize = FMathf::Max(SizeBox->GetWidthOverride(), SizeBox->GetHeightOverride()) * 0.5f;
		float Reciprocal = 1.0f / Element.Value.MiniMapRate;
		Element.Value.BoxExtent.X = Reciprocal * ParentWidgetHalfSize;
		Element.Value.BoxExtent.Y = Element.Value.BoxExtent.X;
		Element.Value.BoxExtent *= MaxOrthoWidth / Element.Value.CaptureMapDatas.OrthoWidth;
	}
	
	UCameraComponent* CameraComponent = GetWorld()->GetFirstPlayerController()->GetPawn()->GetComponentByClass<UCameraComponent>();
	if(CameraComponent)
	{
		FVector ForwardVector= CameraComponent->GetForwardVector();
		double Angle = UKismetMathLibrary::Atan2(ForwardVector.X, ForwardVector.Y);
		Angle = UKismetMathLibrary::RadiansToDegrees(Angle);
		Angle = UKismetMathLibrary::GenericPercent_FloatFloat((Angle + 360.0f), 360.0f);
		Angle = Angle - 90.0f;
		MinimapCanvas->SetRenderTransformAngle(Angle);
	}

	FVector TargetActorLocation = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	for (auto Element : ActiveTileWidgets)
	{
		auto GridInfo = Element.Value.CaptureMapDatas;
		FVector Translation = (TargetActorLocation - (GridInfo.CaptureMapDatas.RootLocation + GridInfo.CaptureMapDatas.RelativeLocation)) * GridInfo.MiniMapRate;
		Translation *= (GridInfo.CaptureMapDatas.OrthoWidth / MaxOrthoWidth);
		float tempY = Translation.Y;
		Translation.Y = Translation.X;
		Translation.X = tempY * -1.0f;
		Element.Value.Image->SetRenderTranslation(FVector2d(Translation.X, Translation.Y));

		UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Element.Value.Image->Slot.Get());
		PanelSlot->SetSize(FVector2D(GetMiniMapSize() * (GridInfo.CaptureMapDatas.OrthoWidth / MaxOrthoWidth), GetMiniMapSize() * (GridInfo.CaptureMapDatas.OrthoWidth / MaxOrthoWidth)));
	}
}

void UUKMiniMapWidget::UnLoadTile()
{
	FVector TargetActorLocation = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	for (int i=ActiveTileWidgets.Num()-1; i >=0; --i)
	{
		auto Element = ActiveTileWidgets.Array()[i];
		auto GridInfo = UICaptureMapInfos.Find(Element.Key);
		if(!GridInfo)
			return;
		
		FVector GridInfoLocation = GridInfo->CaptureMapDatas.RootLocation + GridInfo->CaptureMapDatas.RelativeLocation;
		FTransform GridInfoTransform = FTransform();
		GridInfoTransform.SetLocation(GridInfoLocation);
		if(!UKismetMathLibrary::IsPointInBoxWithTransform(TargetActorLocation, GridInfoTransform, GridInfo->CaptureMapDatas.BoxSize + GridInfo->BoxExtent))
		{
			// MinimapCanvas->RemoveChild(Element.Value.TileWidget.Get());
			Element.Value.Image->RemoveFromParent();
			ActiveTileWidgets.Remove(Element.Key);
		}
	}
}

float UUKMiniMapWidget::GetMiniMapSize()
{
	return MiniMapUISize * MiniMapUIScale;
}