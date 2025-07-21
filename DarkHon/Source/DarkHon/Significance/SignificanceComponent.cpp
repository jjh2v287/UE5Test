// Fill out your copyright notice in the Description page of Project Settings.


#include "SignificanceComponent.h"

#include "DrawDebugHelpers.h"
#include "SceneManagement.h"
#include "SignificanceManager.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"


// Sets default values for this component's properties
USignificanceComponent::USignificanceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void USignificanceComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerActor = GetOwner();
	MovementComponent = OwnerActor->FindComponentByClass<UCharacterMovementComponent>();
	SkeletalMeshComponent = OwnerActor->FindComponentByClass<USkeletalMeshComponent>();
	
	if (const APawn* Pawn = Cast<APawn>(OwnerActor))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			BehaviorTreeComponent = Controller->FindComponentByClass<UBehaviorTreeComponent>();
		}
	}
	
	// Enable URO
	if (SkeletalMeshComponent && bDistanceURO)
	{
		SkeletalMeshComponent->bEnableUpdateRateOptimizations = true;
		
		if (FAnimUpdateRateParameters* AnimUpdateRateParams = SkeletalMeshComponent->AnimUpdateRateParams)
		{
			AnimUpdateRateParams->bShouldUseLodMap = true;
			AnimUpdateRateParams->MaxEvalRateForInterpolation = MaxEvalRateForInterpolation;
			AnimUpdateRateParams->BaseNonRenderedUpdateRate = BaseNonRenderedUpdateRate;
		}
	}
	
	SignificanceCount = SignificanceThresholds.Num();
	SignificanceLastIndex = SignificanceCount - 1; 
	LastLevel = SignificanceThresholds[SignificanceThresholds.Num() - 1].Significance;
	
	auto SignificanceLambda = [this](USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& Viewpoint) -> float
	{
		return SignificanceFunction(ObjectInfo, Viewpoint);
	};

	auto PostSignificanceLambda = [this](USignificanceManager::FManagedObjectInfo* ObjectInfo, const float OldSignificance, const float Significance, const bool bFinal)
	{
		PostSignificanceFunction(ObjectInfo, OldSignificance, Significance, bFinal);
	};

	FName ActorTag = OwnerActor->Tags[0];
	USignificanceManager* SignificanceManager = USignificanceManager::Get(GetWorld());
	SignificanceManager->RegisterObject(GetOwner(), ActorTag, SignificanceLambda, USignificanceManager::EPostSignificanceType::Sequential, PostSignificanceLambda);
}

void USignificanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	USignificanceManager* SignificanceManager = USignificanceManager::Get(GetWorld());

	if (!IsValid(SignificanceManager))
	{
		return;
	}

	SignificanceManager->UnregisterObject(GetOwner());
}

float USignificanceComponent::SignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& Viewpoint)
{
	// 1. ULocalPlayer 가져오기
	// Significance Manager는 보통 로컬 플레이어 기준으로 작동하므로, PlayerController를 거칠 필요 없이 바로 가져올 수 있습니다.
	ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		// 서버 환경이나 로컬 플레이어가 없는 경우, 기본 중요도나 다른 로직을 사용할 수 있습니다.
		return 0.0f;
	}

	// 이제 ViewInfo를 사용하여 투영 행렬(Projection Matrix)을 계산합니다.
	const FVector CamLocation = Viewpoint.GetLocation(); // 파라미터로 받은 Viewpoint를 사용
	
	// 2. 액터의 경계(Bounds)와 화면 크기 계산
	FBox OutRuntimeBounds;
	FBox OutEditorBounds;
	OwnerActor->GetStreamingBounds(OutRuntimeBounds,OutEditorBounds); // 혹은 GetActorBounds(false, Origin, Extent);
	const FBoxSphereBounds Bounds = OutRuntimeBounds;

	const FViewport* Viewport = LocalPlayer->ViewportClient->Viewport;
	const FIntPoint ViewportSize = Viewport->GetSizeXY();
	const float ScreenX = ViewportSize.X;
	const float ScreenY = ViewportSize.Y;
	float FullFOV = 90.f; // 기본값
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
		{
			FullFOV = CameraManager->GetFOVAngle();
		}
	}
	const float HalfFOVRad = FMath::DegreesToRadians(FullFOV * 0.5f);

	
	// static const float ScreenX = 1920;
	// static const float ScreenY = 1080;
	// static const float HalfFOVRad = FMath::DegreesToRadians(55.0f);
	static const FMatrix ProjectionMatrix = FPerspectiveMatrix(HalfFOVRad, ScreenX, ScreenY, 0.01f);

	
	// 이 함수가 핵심입니다. 액터의 경계 볼륨(Bounding Box)이 화면 공간(Screen Space)에서 차지하는 크기를 계산해 줍니다.
	const float ScreenSize = ComputeBoundsScreenSize(
		Bounds.Origin,
		Bounds.SphereRadius,
		CamLocation,
		ProjectionMatrix
	);

	// 3. 계산된 화면 크기(ScreenSize)를 기준으로 최적화 레벨 결정
	if (ScreenSize > 0.15f)
	{
		// 화면 높이의 15% 이상 차지 (매우 가까움)
		// NPC->SetOptimizationLevel(ENPCOptimizationLevel::Near);
	}
	else if (ScreenSize > 0.04f) // 4% 이상 (중간 거리)
	{
		// 4% 이상 (중간 거리)
		// NPC->SetOptimizationLevel(ENPCOptimizationLevel::MidDistance);
	}
	else if (ScreenSize > 0.0f)
	{
		// 화면에 픽셀이라도 보임 (화면 안, 하지만 매우 작음)
		// 애니메이션은 멈추되 AI는 꺼도 좋음
	}
	else
	{
		// 화면에 보이지 않음 (Frustum Culling)
	}
	
	const FVector Direction = OwnerActor->GetActorLocation() - Viewpoint.GetLocation();
	float Distance = Direction.Size();

	const bool bFrontView = FVector::DotProduct(Viewpoint.GetRotation().GetForwardVector(), Direction) > 0;
	Distance = bFrontView ? Distance : Distance * 10.0f;
	return GetSignificanceByDistance(Distance);

	/*// 탑다운 뷰에서는 Z축(높이)을 무시한 2D 거리를 사용하는 것이 더 적합할 수 있습니다.
	const float Distance = FVector::Dist2D(OwnerActor->GetActorLocation(), Viewpoint.GetLocation());

	// 만약 높이(Z)까지 포함한 3D 거리가 필요하다면 아래 코드를 사용하세요.
	// const float Distance = (OwnerActor->GetActorLocation() - Viewpoint.GetLocation()).Size();

	return GetSignificanceByDistance(Distance);*/

}

float USignificanceComponent::GetSignificanceByDistance(const float Distance)
{
	if (Distance >= SignificanceThresholds[SignificanceLastIndex].MaxDistance)
	{
		return SignificanceThresholds[SignificanceLastIndex].Significance;
	}

	for (int32 i = 0; i < SignificanceCount; ++i)
	{
		if (Distance <= SignificanceThresholds[i].MaxDistance)
		{
			return SignificanceThresholds[i].Significance;
		}
	}
	
	return 0.0f;
}

void USignificanceComponent::PostSignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
{
	if (Significance >= LastLevel)
	{
		SetTickEnabled(false);
	}
	else
	{
		SetTickEnabled(true);

		const float TickInterval = SignificanceThresholds[Significance].TickInterval;

		if (MovementComponent)
		{
			MovementComponent->SetComponentTickInterval(TickInterval);	
		}

		if (SkeletalMeshComponent)
		{
			SkeletalMeshComponent->SetComponentTickInterval(TickInterval);
			if (FAnimUpdateRateParameters* AnimUpdateRateParams = SkeletalMeshComponent->AnimUpdateRateParams)
			{
				int32& FrameSkipCount = AnimUpdateRateParams->LODToFrameSkipMap.FindOrAdd(0);
				FrameSkipCount = SignificanceThresholds[Significance].AnimationFrameSkipCount;
			}
		}

		if (BehaviorTreeComponent)
		{
			BehaviorTreeComponent->SetComponentTickInterval(TickInterval);	
		}
	}

	DrawDebugText(Significance);
}


void USignificanceComponent::SetTickEnabled(const bool bEnabled)
{
	OwnerActor->SetActorTickEnabled(bEnabled);
	
	if (MovementComponent)
	{
		MovementComponent->SetComponentTickEnabled(bEnabled);	
	}

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetComponentTickEnabled(bEnabled);	
	}
}

void USignificanceComponent::DrawDebugText(const float Significance) const
{	
	FColor DebugColor = FColor::White;

	if (Significance >= LastLevel)
	{
		DebugColor = FColor::Green;
	}
	else if (Significance >= 1.0f)
	{
		DebugColor = FColor::Yellow;
	}
	
	FVector Location = OwnerActor->GetActorLocation();
	FString ClassName = OwnerActor->GetClass()->GetName();
	FName ActorTag = OwnerActor->Tags[0];
	const FString DebugMsg = FString::Printf(TEXT("Sig[%.1f]\nTag[%s]"), Significance, *ActorTag.ToString());
	Location.Z += 50.0f;
	DrawDebugString(GetWorld(), Location, DebugMsg, nullptr, DebugColor,  1.0f, true);	
}