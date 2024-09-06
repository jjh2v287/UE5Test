// Fill out your copyright notice in the Description page of Project Settings.


#include "UKWidget.h"
#include "GameFramework/Character.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"

void UUKWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0);
	UCanvasPanelSlot* CanvasPanelSlot = Cast<UCanvasPanelSlot>(Slot);
	
	// Set Widget Position
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this) / ViewportScale;
	const FVector2D WidgetSize = GetDesiredSize() * 0.5f;

	FVector WorldLocation = FVector(100000.0f, 200.0f, 170.0f);
	
	bool bResult = false;
	FVector2D ScreenPosition;
	// UGameplayStatics::ProjectWorldToScreen(Player, WorldLocation, ScreenPosition, false);
	ULocalPlayer* const LP = Player ? Player->GetLocalPlayer() : nullptr;
	if (LP && LP->ViewportClient)
	{
		// 이건 화면 밖일때에도 계산하게 처리 bShouldCalcOutsideViewPosition = true
		FSceneViewProjectionData ProjectionData;
		if (LP->GetProjectionData(LP->ViewportClient->Viewport, ProjectionData))
		{
			FMatrix const ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();
			bResult = FSceneView::ProjectWorldToScreen(WorldLocation, ProjectionData.GetConstrainedViewRect(), ViewProjectionMatrix, ScreenPosition, true);
		}
	}
	
	const float ClampX = true ? WidgetSize.X : -WidgetSize.X;
	const float ClampY = true ? WidgetSize.Y : -WidgetSize.Y;
	
	FVector2D NewScreenPosition = ScreenPosition / ViewportScale;
	NewScreenPosition.X = FMath::Clamp(NewScreenPosition.X, ClampX, ViewportSize.X + -ClampX);
	NewScreenPosition.Y = FMath::Clamp(NewScreenPosition.Y, ClampY, ViewportSize.Y + -ClampY);
	// 이건 밑으로 내리기
	NewScreenPosition.Y = !bResult ? ViewportSize.Y + -ClampY : NewScreenPosition.Y;
	
	Marker->SetRenderTranslation(NewScreenPosition);
	// CanvasPanelSlot->SetPosition(ScreenPosition);

	FMinimalViewInfo ViewInfo;
	ACharacter* ControlledCharacter = Cast<ACharacter>(Player->GetPawn());
	UCameraComponent* CameraComponent = ControlledCharacter->FindComponentByClass<UCameraComponent>();
	CameraComponent->GetCameraView(0.0f, ViewInfo);
	
	// 1. 카메라의 뷰 행렬과 프로젝션 행렬 가져오기
	FMatrix ViewMatrix = Player->PlayerCameraManager->GetViewTarget()->GetActorTransform().ToInverseMatrixWithScale();
	FMatrix ProjectionMatrix = ViewInfo.CalculateProjectionMatrix();
	
	// 2. 월드 좌표를 뷰 좌표로 변환
	FVector4 ViewLocation = ViewMatrix.TransformPosition(WorldLocation);

	// 3. 뷰 좌표를 클립 좌표로 변환
	FVector4 ClipLocation = ProjectionMatrix.TransformFVector4(ViewLocation);

	// 4. 정규 장치 좌표(NDC)로 변환
	FVector2D NDCPosition = FVector2D(ClipLocation.X / ClipLocation.W, ClipLocation.Y / ClipLocation.W);

	// 5. NDC를 스크린 좌표로 변환
	FVector2D ScreenPosition2;
	ScreenPosition2.X = (NDCPosition.X + 1.0f) * 0.5f * ViewportSize.X;
	ScreenPosition2.Y = (1.0f - NDCPosition.Y) * 0.5f * ViewportSize.Y; // Y축 반전
	
	// CanvasPanelSlot->SetPosition(ScreenPosition2);
}

// void UUKWidget::FindScreenEdgeLocationForWorldLocation(UObject* WorldContextObject, const FVector& InLocation, const float EdgePercent,  FVector2D& OutScreenPosition, float& OutRotationAngleDegrees, bool &bIsOnScreen)
// {
// 	bIsOnScreen = false;
// 	OutRotationAngleDegrees = 0.f;
// 	FVector2D *ScreenPosition = new FVector2D();
// 	
// 	const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
// 	const FVector2D  ViewportCenter =  FVector2D(ViewportSize.X/2, ViewportSize.Y/2);
// 	
// 	APlayerController* PlayerController = (WorldContextObject ? UGameplayStatics::GetPlayerController(WorldContextObject, 0) : NULL);
// 	ACharacter *PlayerCharacter = static_cast<ACharacter *> (PlayerController->GetPawn());
// 	
// 	if (!PlayerCharacter) return;
// 	
// 	
// 	FVector Forward = PlayerCharacter->GetActorForwardVector();
// 	FVector Offset = (InLocation - PlayerCharacter->GetActorLocation()).SafeNormal();
// 	
// 	float DotProduct = FVector::DotProduct(Forward, Offset);
// 	bool bLocationIsBehindCamera = (DotProduct < 0);
// 	
// 	if (bLocationIsBehindCamera)
// 	{
// 		// For behind the camera situation, we cheat a little to put the
// 		// marker at the bottom of the screen so that it moves smoothly
// 		// as you turn around. Could stand some refinement, but results
// 		// are decent enough for most purposes.
// 		
// 		FVector DiffVector = InLocation - PlayerCharacter->GetActorLocation();
// 		FVector Inverted = DiffVector * -1.f;
// 		FVector NewInLocation = PlayerCharacter->GetActorLocation() * Inverted;
// 		
// 		NewInLocation.Z -= 5000;
// 		
// 		PlayerController->ProjectWorldLocationToScreen(NewInLocation, *ScreenPosition);
// 		ScreenPosition->Y = (EdgePercent * ViewportCenter.X) * 2.f;
// 		ScreenPosition->X = -ViewportCenter.X - ScreenPosition->X;
// 	}
// 	
// 	PlayerController->ProjectWorldLocationToScreen(InLocation, *ScreenPosition);
// 	
// 	// Check to see if it's on screen. If it is, ProjectWorldLocationToScreen is all we need, return it.
// 	if (ScreenPosition->X >= 0.f && ScreenPosition->X <= ViewportSize.X
// 		&& ScreenPosition->Y >= 0.f && ScreenPosition->Y <= ViewportSize.Y)
// 	{
// 		OutScreenPosition = *ScreenPosition;
// 		bIsOnScreen = true;
// 		return;
// 	}
// 	
// 	*ScreenPosition -= ViewportCenter;
//
// 	float AngleRadians = FMath::Atan2(ScreenPosition->Y, ScreenPosition->X);
// 	AngleRadians -= FMath::DegreesToRadians(90.f);
// 	
// 	OutRotationAngleDegrees = FMath::RadiansToDegrees(AngleRadians) + 180.f;
// 	
// 	float Cos = cosf(AngleRadians);
// 	float Sin = -sinf(AngleRadians);
// 	
// 	ScreenPosition = new FVector2D(ViewportCenter.X + (Sin * 150.f), ViewportCenter.Y + Cos * 150.f);
// 	
// 	float m = Cos / Sin;
// 	
// 	FVector2D ScreenBounds = ViewportCenter * EdgePercent;
// 	
// 	if (Cos > 0)
// 	{
// 		ScreenPosition = new FVector2D(ScreenBounds.Y/m, ScreenBounds.Y);
// 	}
// 	else
// 	{
// 		ScreenPosition = new FVector2D(-ScreenBounds.Y/m, -ScreenBounds.Y);
// 	}
// 	
// 	if (ScreenPosition->X > ScreenBounds.X)
// 	{
// 		ScreenPosition = new FVector2D(ScreenBounds.X, ScreenBounds.X*m);
// 	}
// 	else if (ScreenPosition->X < -ScreenBounds.X)
// 	{
// 		ScreenPosition = new FVector2D(-ScreenBounds.X, -ScreenBounds.X*m);
// 	}
// 	
// 	*ScreenPosition += ViewportCenter;
// 	
// 	OutScreenPosition = *ScreenPosition;
// 	
// }