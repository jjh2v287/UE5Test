// Fill out your copyright notice in the Description page of Project Settings.


#include "ZKPlayerController.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AZKPlayerController::AZKPlayerController(const FObjectInitializer& ObjectInitializer)
   :Super(ObjectInitializer)
{
   bShowMouseCursor = false;
}

void AZKPlayerController::BeginPlay()
{
   Super::BeginPlay();

   // Enhanced Input 초기화
   if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
   {
       Subsystem->AddMappingContext(DefaultMappingContext, 0);
   }
}

void AZKPlayerController::SetupInputComponent()
{
   Super::SetupInputComponent();

   if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
   {
       EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AZKPlayerController::Move);
       EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AZKPlayerController::Look);
       EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AZKPlayerController::Jump);
   }
}

void AZKPlayerController::Move(const FInputActionValue& Value)
{
   // input is a Vector2D
   FVector2D MovementVector = Value.Get<FVector2D>();

   if (GetCharacter() != nullptr)
   {
      // find out which way is forward
      const FRotator Rotation = GetControlRotation();
      const FRotator YawRotation(0, Rotation.Yaw, 0);

      // get forward vector
      const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
      // get right vector 
      const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

      // add movement 
      GetCharacter()->AddMovementInput(ForwardDirection, MovementVector.Y);
      GetCharacter()->AddMovementInput(RightDirection, MovementVector.X);
   }
}

void AZKPlayerController::Look(const FInputActionValue& Value)
{
   const FVector2D LookAxisVector = Value.Get<FVector2D>();
   
   AddYawInput(LookAxisVector.X);
   AddPitchInput(LookAxisVector.Y);
}

void AZKPlayerController::Jump(const FInputActionValue& Value)
{
   if (ACharacter* PlayerCharacter = Cast<ACharacter>(GetPawn()))
   {
       PlayerCharacter ->Jump();
   }
}