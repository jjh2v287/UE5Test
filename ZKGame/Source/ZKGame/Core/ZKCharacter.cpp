#include "ZKCharacter.h"

#include "AbilitySystemComponent.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ZKAbilitySystemComponent.h"

// Constructor
AZKCharacter::AZKCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    /* 나중에 추가할 코드에 대한 힌트
        GetCapsuleComponent()->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.0f));
        UKCharacterMovementComponent = Cast<UUKCharacterMovementComponent>(GetCharacterMovement());
        UKSkeletalMeshComponent = Cast<UUKCharacterSkeletalMeshComponent>(GetMesh());
        AnimInstance = Cast<UUKCharacterAnimInstance>(GetMesh()->GetAnimInstance());
        CapsuleModifierComponent = CreateDefaultSubobject<UUKCapsuleModifierComponent>(TEXT("CapsuleModifierComponent"));
        AbilitySystemComponent = CreateOptionalDefaultSubobject<UUKAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    */
    
    PrimaryActorTick.bCanEverTick = true;

    // Character rotation settings
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
    MovementComponent->bOrientRotationToMovement = true;
    MovementComponent->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    MovementComponent->JumpZVelocity = 700.f;
    MovementComponent->AirControl = 0.35f;
    MovementComponent->MaxWalkSpeed = 500.f;
    MovementComponent->MinAnalogWalkSpeed = 20.f;
    MovementComponent->BrakingDecelerationWalking = 2000.f;
    MovementComponent->BrakingDecelerationFalling = 1500.0f;

    // Create camera boom
    CameraBoom = ObjectInitializer.CreateDefaultSubobject<USpringArmComponent>(this, TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    // Create follow camera
    FollowCamera = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // Setup ability system
    AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UZKAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

// Override Functions
void AZKCharacter::BeginPlay()
{
    Super::BeginPlay();
    AddStartupGameplayAbilities();
}

void AZKCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsRotating)
    {
        FRotator NewRotation = FMath::RInterpConstantTo(GetActorRotation(), TargetRotation, DeltaTime, CurrentRotationRate);
        SetActorRotation(NewRotation);

        if (FMath::IsNearlyEqual(GetActorRotation().Yaw, TargetRotation.Yaw, 1.0f))
        {
            SetActorRotation(TargetRotation);
            bIsRotating = false;
        }
    }
}

UAbilitySystemComponent* AZKCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AZKCharacter::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();

    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void AZKCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AZKCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AZKCharacter::Look);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Started, this, &AZKCharacter::Roll);
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AZKCharacter::Attack);
    }
}

// Input Functions
void AZKCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);

        if (!MovementVector.IsNearlyZero())
        {
            FVector MovementDirection = ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X;
            MovementDirection.Normalize();
            
            TargetRotation = MovementDirection.Rotation();
            CalculateRotationRate();
            bIsRotating = true;
        }
    }
}

void AZKCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void AZKCharacter::Roll()
{
    OnRollEvent();
}

void AZKCharacter::Attack()
{
    OnAttackEvent();
}

// Gameplay Ability Functions
void AZKCharacter::AddStartupGameplayAbilities()
{
    for (TSubclassOf<UGameplayAbility>& StartupAbility : GameplayAbilities)
    {
        AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(StartupAbility, 1, INDEX_NONE, this));
    }
}

// Rotation Function
void AZKCharacter::CalculateRotationRate()
{
    float DeltaYaw = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, TargetRotation.Yaw);
    float AbsDeltaYaw = FMath::Abs(DeltaYaw);
    
    float TargetRotationTime;
    if (AbsDeltaYaw <= 90.0f)
    {
        TargetRotationTime = 0.15f * (AbsDeltaYaw / 90.0f);
    }
    else
    {
        float ExtraAngle = AbsDeltaYaw - 90.0f;
        float ExtraTime = (0.2f - 0.15f) * (ExtraAngle / 90.0f);
        TargetRotationTime = 0.15f + ExtraTime;
    }
    
    CurrentRotationRate = AbsDeltaYaw / TargetRotationTime;
}