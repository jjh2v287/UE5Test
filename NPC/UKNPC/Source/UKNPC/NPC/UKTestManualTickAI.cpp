// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKTestManualTickAI.h"
#include "BrainComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "UKNPCUpdateManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKTestManualTickAI)

AUKTestManualTickAI::AUKTestManualTickAI()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AUKTestManualTickAI::BeginPlay()
{
	Super::BeginPlay();
	// 1. 엔진 틱 비활성화 (매니저가 부를 것이므로)
	SetActorTickEnabled(false);
	
	// 2. 캐릭터 무브먼트 틱 끄기 (이동 연산 정지)
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetComponentTickEnabled(false);
		// GetCharacterMovement()->DestroyComponent();
	}
	

	// 3. 스켈레탈 메쉬 틱 끄기 (애니메이션 연산 정지)
	if (GetMesh())
	{
		GetMesh()->SetComponentTickEnabled(false);
		// 옵션: 아예 포즈 업데이트도 막고 싶다면 아래 설정 추가
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}
	
	// SMComponent = FindComponentByClass<UUKSimpleMovementComponent>();
	// if (SMComponent)
	// {
	// 	SMComponent->SetComponentTickEnabled(false);
	// }

	// 4. AI 컨트롤러 틱 끄기 (두뇌 정지)
	if (Controller)
	{
		// 컨트롤러는 액터이므로 ActorTick을 끕니다.
		Controller->SetActorTickEnabled(false);
	}

	// 매니저 등록
	if (auto* Manager = GetWorld()->GetSubsystem<UUKNPCUpdateManager>())
	{
		Manager->Register(this);
	}
}

void AUKTestManualTickAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AUKTestManualTickAI::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AUKTestManualTickAI::ManualTick(float DeltaTime)
{
	// ====================================================
	// 1. AI 업데이트 (Brain, Perception 등)
	// ====================================================
	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AICon->GetBrainComponent())
		{
			if (Brain->IsActive())
			{
				Brain->TickComponent(DeltaTime, LEVELTICK_All, nullptr);
			}
		}
	}

	// ====================================================
	// 2. 이동 (Movement) 업데이트
	// ====================================================
	if (GetCharacterMovement() && GetCharacterMovement()->IsActive())
	{
		GetCharacterMovement()->TickComponent(DeltaTime, LEVELTICK_All, nullptr);
	}
	
	if (SMComponent)
	{
		SMComponent->TickComponent(DeltaTime, LEVELTICK_All, nullptr);
	}

	// ====================================================
	// 3. 애니메이션 (Mesh) 업데이트
	// ====================================================
	if (GetMesh() && GetMesh()->IsActive())
	{
		// 화면에 보일 때만 업데이트 하는 것이 성능상 이득
		if (GetMesh()->WasRecentlyRendered(0.2f)) 
		{
			GetMesh()->TickComponent(DeltaTime, LEVELTICK_All, nullptr);
		}
		else
		{
			// 안 보이면 애니메이션은 건너뛰거나, 아주 가끔만 업데이트
			// 필요하다면 본 트랜스폼만 갱신
			// GetMesh()->RefreshBoneTransforms();
		}
	}
	
	// ====================================================
	// 4. 액터 자체 로직 (필요하다면)
	// ====================================================
	// Super::Tick(DeltaTime); // 필요시에만
}

