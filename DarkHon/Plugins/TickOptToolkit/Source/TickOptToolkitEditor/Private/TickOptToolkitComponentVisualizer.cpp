// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkitComponentVisualizer.h"
#include "TickOptToolkitTargetComponent.h"
#include "TickOptToolkitSettings.h"

#include "SceneManagement.h"

static void DrawSphereZone(FPrimitiveDrawInterface* PDI, const FVector& Location, float Radius, const FLinearColor& Color)
{
	const int32 Sides = FMath::Clamp<int32>(Radius / 4.f, 16, 64);
	DrawCircle(PDI, Location, FVector::RightVector, FVector::ForwardVector, Color, Radius, Sides, SDPG_World);
	DrawCircle(PDI, Location, FVector::RightVector, FVector::UpVector, Color, Radius, Sides, SDPG_World);
	DrawCircle(PDI, Location, FVector::ForwardVector, FVector::UpVector, Color, Radius, Sides, SDPG_World);
}

static void DrawBoxZone(FPrimitiveDrawInterface* PDI, const FVector& Location, const FVector& XAxis, const FVector& YAxis, const FVector& Extents, const FLinearColor& Color)
{
	DrawOrientedWireBox(PDI, Location, XAxis, YAxis, FVector::UpVector, Extents, Color, SDPG_World);	
}

void FTickOptToolkitComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UTickOptToolkitTargetComponent* TickOptToolkitComponent = Cast<UTickOptToolkitTargetComponent>(Component);
	if (TickOptToolkitComponent == nullptr || TickOptToolkitComponent->GetOwner() == nullptr)
	{
		return;
	}

	if (UTickOptToolkitSettings::StaticClass()->GetDefaultObject<UTickOptToolkitSettings>()->bLimitTargetVisualizations)
	{
		static uint32 LastFrame = 0;
		static TMap<const FSceneView*, const UActorComponent*> ComponentThisFrame;
		if (LastFrame != GFrameNumber)
		{
			LastFrame = GFrameNumber;
			ComponentThisFrame.Reset();
		}
		if (const UActorComponent** ComponentForView = ComponentThisFrame.Find(View))
		{
			if (*ComponentForView != Component)
			{
				return;
			}
		}
		else
		{
			ComponentThisFrame.Add(View, Component);
		}
	}
	
	const FVector Location = TickOptToolkitComponent->GetComponentLocation();
	const TArray<float>& MidZones = TickOptToolkitComponent->GetMidZoneSizes();
	
	const FLinearColor LightColor = FLinearColor::Yellow;
	const FLinearColor DarkColor = LightColor * 0.2f;
	
	switch (TickOptToolkitComponent->GetDistanceMode())
	{
	case ETickOptToolkitDistanceMode::None:
		break;
		
	case ETickOptToolkitDistanceMode::Sphere:
		{
			float Radius = TickOptToolkitComponent->GetSphereRadius();
			DrawSphereZone(PDI, Location, Radius, LightColor);
			for (int ZI = 0; ZI < MidZones.Num(); ++ZI)
			{
				FLinearColor Color = FMath::Lerp(LightColor, DarkColor, (ZI + 1.0f) / MidZones.Num());
				Radius += MidZones[ZI];
				DrawSphereZone(PDI, Location, Radius, Color);
			}
			break;
		}
		
	case ETickOptToolkitDistanceMode::Box:
		{
			FVector Extents = TickOptToolkitComponent->GetBoxExtents();
			
			const float Rotation = TickOptToolkitComponent->GetBoxRotation();
			const FVector XAxis = FVector::RightVector.RotateAngleAxis(Rotation, FVector::UpVector);
			const FVector YAxis = FVector::ForwardVector.RotateAngleAxis(Rotation, FVector::UpVector);
			
			DrawBoxZone(PDI, Location, XAxis, YAxis, Extents, LightColor);
			for (int ZI = 0; ZI < MidZones.Num(); ++ZI)
			{
				FLinearColor Color = FMath::Lerp(LightColor, DarkColor, (ZI + 1.0f) / MidZones.Num());
				Extents = Extents + MidZones[ZI];
				DrawBoxZone(PDI, Location, XAxis, YAxis, Extents, Color);
			}
			break;
		}
	}
}
