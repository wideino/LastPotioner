// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/LPCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapons/LPWeapon.h"
#include "Components/LPAttributeComponent.h"
#include "GameFramework/PlayerController.h"

ALPCharacter::ALPCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	SpringArm1 = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm1->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm1);

	GetCharacterMovement()->bOrientRotationToMovement = true;

	AttributeComponent = CreateDefaultSubobject<ULPAttributeComponent>(TEXT("Attribute Component"));

	GetMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
}

void ALPCharacter::BeginPlay()
{
	Super::BeginPlay();

	Tags.Add(FName("SlashCharacter"));

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(LPMappingContext, 0);
		}
	}
}

void ALPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent =
		CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) // game ends if cast failed
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALPCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALPCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &ALPCharacter::Attack);
		EnhancedInputComponent->BindAction(
			EquipAction, ETriggerEvent::Triggered, this, &ALPCharacter::ArmWeapon);
		EnhancedInputComponent->BindAction(
			PickUpAction, ETriggerEvent::Triggered, this, &ALPCharacter::PickUpOverlappingItem);
	}
}

void ALPCharacter::Move(const FInputActionValue& Value)
{
	//if (ActionState != EActionState::EAS_Unoccupied) return;

	const FVector2D InputVector = Value.Get<FVector2D>();

	if (!InputVector.IsNearlyZero())
	{
		const FRotator ControlRotation = GetControlRotation();
		const FRotator YawControlRotation = FRotator(0.0, ControlRotation.Yaw, 0.0);

		const FVector ForwardVector = FRotationMatrix(YawControlRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(ForwardVector, InputVector.X * GetWorld()->GetDeltaSeconds() * Speed);

		const FVector RightVector = FRotationMatrix(YawControlRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(RightVector, InputVector.Y * GetWorld()->GetDeltaSeconds() * Speed);
	}
}

void ALPCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisValue = Value.Get<FVector2D>();

	if (!LookAxisValue.IsZero() && GetController() && GetController()->IsLocalPlayerController())
	{
		AddControllerYawInput(LookAxisValue.X * RotationSensitivityMultiplier);
		AddControllerPitchInput(-LookAxisValue.Y * RotationSensitivityMultiplier);
	}
}

void ALPCharacter::PickUpOverlappingItem()
{
	if (ALPWeapon* OverlappingWeapon = Cast<ALPWeapon>(OverlappingItem))
	{
		OverlappingWeapon->Equip(GetMesh(), this, this);
		EquippedWeapons.Add(OverlappingWeapon);
		OverlappingItem = nullptr;
	}
}

void ALPCharacter::ContinueArm()
{
	PlayAnimMontage(EquipAnimMontage, 1.0f, FName("Equip"));
	CurrentWeapon = EquippedWeapons[CurrentWeaponPosition];

	switch (CurrentWeapon->GetWeaponType())
	{
	case EWeaponType::EWT_OneHanded:
		CharacterState = ECharacterState::ECS_EquippedOneHandedWeapon;
		break;
	case EWeaponType::EWT_TwoHanded:
		CharacterState = ECharacterState::ECS_EquippedTwoHandedWeapon;
		break;
	}

	OnCharacterStateChanged.Broadcast(CharacterState);
}

void ALPCharacter::ArmWeapon()
{
	if (EquippedWeapons.Num() == 0 || ActionState != EActionState::EAS_Unoccupied) return;
	if (EquippedWeapons.Num() == 1 && CurrentWeapon) return; // if the only weapon is equipped

	ActionState = EActionState::EAS_ArmingWeapon;

	CurrentWeaponPosition = 0;
	if (CurrentWeapon)
	{
		CurrentWeaponPosition = EquippedWeapons.IndexOfByKey(CurrentWeapon);
		CurrentWeaponPosition = (CurrentWeaponPosition + 1) % EquippedWeapons.Num();
		ActionState = EActionState::EAS_DisarmingBeforeArming;
		DisarmWeapon();
		return;
	}

	ContinueArm();
}

void ALPCharacter::DisarmWeapon()
{
	if (!CurrentWeapon) return;
	if (ActionState != EActionState::EAS_DisarmingBeforeArming
		&& ActionState != EActionState::EAS_Unoccupied)
		return;

	if (ActionState != EActionState::EAS_DisarmingBeforeArming)
	{
		ActionState = EActionState::EAS_DisarmingWeapon;
	}

	PlayAnimMontage(EquipAnimMontage, 1.0f, FName("Unequip"));
}

void ALPCharacter::DisarmEnd()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Disarm();
		CurrentWeapon = nullptr;

		if (ActionState == EActionState::EAS_DisarmingBeforeArming)
		{
			ContinueArm();
			return;
		}

		ActionState = EActionState::EAS_Unoccupied;
		CharacterState = ECharacterState::ECS_Unequipped;
		OnCharacterStateChanged.Broadcast(CharacterState);
	}
}

void ALPCharacter::ArmStart()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Arm();
		ActionState = EActionState::EAS_Unoccupied;
	}
}

void ALPCharacter::Attack()
{
	if (CanAttack())
	{
		//GetCharacterMovement()->SetMovementMode(MOVE_None);
		ActionState = EActionState::EAS_Attacking;
		PlayAttackMontage();
	}
}

void ALPCharacter::AttackEnd()
{
	//GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	ActionState = EActionState::EAS_Unoccupied;
}

bool ALPCharacter::CanAttack() const
{
	return (ActionState == EActionState::EAS_Unoccupied) && (CharacterState != ECharacterState::ECS_Unequipped);
}

void ALPCharacter::GetHit_Implementation(const FHitResult& HitResult)
{
	Super::GetHit_Implementation(HitResult);
}