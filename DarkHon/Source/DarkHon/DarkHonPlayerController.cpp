// Copyright Epic Games, Inc. All Rights Reserved.

#include "DarkHonPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "DarkHonCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ADarkHonPlayerController::ADarkHonPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
}

void ADarkHonPlayerController::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

void ADarkHonPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Setup mouse input events
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &ADarkHonPlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &ADarkHonPlayerController::OnSetDestinationTriggered);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &ADarkHonPlayerController::OnSetDestinationReleased);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &ADarkHonPlayerController::OnSetDestinationReleased);

		// Setup touch input events
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this, &ADarkHonPlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this, &ADarkHonPlayerController::OnTouchTriggered);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this, &ADarkHonPlayerController::OnTouchReleased);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this, &ADarkHonPlayerController::OnTouchReleased);

		// Setup jump events
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ADarkHonPlayerController::RequestJump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ADarkHonPlayerController::RequestStopJump);
		}

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADarkHonPlayerController::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ADarkHonPlayerController::MoveReleased);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ADarkHonPlayerController::MoveReleased);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ADarkHonPlayerController::OnInputStarted()
{
	StopMovement();
}

// Triggered every frame when the input is held down
void ADarkHonPlayerController::OnSetDestinationTriggered()
{
	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();
	
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	if (bIsTouch)
	{
		bHitSuccessful = GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
	}
	else
	{
		bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	}

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}
	
	// Move towards mouse pointer or touch
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void ADarkHonPlayerController::OnSetDestinationReleased()
{
	// If it was a short press
	if (FollowTime <= ShortPressThreshold)
	{
		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}

	FollowTime = 0.f;
}

// Triggered every frame when the input is held down
void ADarkHonPlayerController::OnTouchTriggered()
{
	bIsTouch = true;
	OnSetDestinationTriggered();
}

void ADarkHonPlayerController::OnTouchReleased()
{
	bIsTouch = false;
	OnSetDestinationReleased();
}

void ADarkHonPlayerController::RequestJump()
{
	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		ControlledCharacter->Jump();
	}

}

void ADarkHonPlayerController::RequestStopJump()
{
	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		ControlledCharacter->StopJumping();
	}
}

void ADarkHonPlayerController::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D MovementVector = Value.Get<FVector2D>();
	const float Right = MovementVector.X;
	const float Forward  = MovementVector.Y;

	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		// find out which way is forward
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		ControlledCharacter->AddMovementInput(ForwardDirection, Forward);
		ControlledCharacter->AddMovementInput(RightDirection, Right);

		// 입력값을 저장 (나중에 PlayerTick에서 사용)
		CurrentInputVector = MovementVector;
	}
}

void ADarkHonPlayerController::MoveReleased()
{
	CurrentInputVector = FVector2D::ZeroVector;
}

void ADarkHonPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		FHitResult HitResult;
		// 마우스 커서 위치에 대한 라인 트레이스를 수행합니다.
		if (GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, HitResult))
		{
			FVector CursorLocation = HitResult.Location;
			FVector CharacterLocation = ControlledPawn->GetActorLocation();

			// 캐릭터에서 마우스를 향하는 방향 벡터를 계산합니다.
			FVector LookDirection = CursorLocation - CharacterLocation;
			LookDirection.Z = 0.f; // 캐릭터가 기울어지지 않도록 Z축 값을 0으로 설정합니다.

			// 새로운 회전 값을 설정합니다.
			LookDirection = LookDirection.GetSafeNormal();
			ControlledPawn->SetActorRotation(LookDirection.Rotation());

			// Blend Space용 로컬 좌표 계산
			if (CurrentInputVector.Size() > 0.1f) // 입력이 있을 때만
			{
				// 월드 공간에서의 실제 이동 방향
				// Move() 함수에서 사용한 ControlRotation을 가져와 이동 방향을 재구성합니다.
				const FRotator ControlYawRotation(0, GetControlRotation().Yaw, 0);
				const FVector WorldForwardFromInput = FRotationMatrix(ControlYawRotation).GetUnitAxis(EAxis::X);
				const FVector WorldRightFromInput = FRotationMatrix(ControlYawRotation).GetUnitAxis(EAxis::Y);
                
				FVector WorldMovementDirection = (WorldForwardFromInput * CurrentInputVector.Y + WorldRightFromInput * CurrentInputVector.X).GetSafeNormal();

				// 캐릭터가 현재 바라보는 방향 벡터 (정규화)
				const FVector CharacterForward = LookDirection.GetSafeNormal();
				// 캐릭터의 오른쪽 벡터 (외적 및 정규화)
				const FVector CharacterRight = FVector::CrossProduct(FVector::UpVector, CharacterForward).GetSafeNormal(); // Up과 Forward 순서가 더 직관적일 수 있습니다. 결과는 같습니다.
                
				// 내적을 통해 각 축의 값을 계산
				BlendSpaceInput.X = FVector::DotProduct(WorldMovementDirection, CharacterForward); // 앞(1)/뒤(-1)
				BlendSpaceInput.Y = FVector::DotProduct(WorldMovementDirection, CharacterRight);   // 우(1)/좌(-1)
			}
			else
			{
				BlendSpaceInput = FVector2D::ZeroVector; // 입력이 없으면 Idle
			}
		}
	}
}
