// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Player/UT1Player.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UT1/CJW/Weapons/UT1WeaponData.h"

AUT1Player::AUT1Player()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeRotation(FRotator(-50.0f, 45.0f, 0.0f));
	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->TargetArmLength = 800.0f;

	SpringArm->TargetArmLength = 400.0f;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	bUseControllerRotationYaw = false;
}

void AUT1Player::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		auto* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

}

void AUT1Player::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);


	auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this,&AUT1Player::Input_Move);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AUT1Player::ComboAttack);
	}
}

void AUT1Player::ComboAttack()
{
	if (bIsAttacking)
	{
		bComboQueued = true;
		return;
	}

	ComboIndex = 0;
	PlayComboStep();
}

void AUT1Player::PlayComboStep()
{
	const TArray<FComboStep>& Combo = TestWeaponData->ComboSequence;
	if (Combo.IsValidIndex(ComboIndex) == false)
	{
		ComboReset();
		return;
	}

	UAnimMontage* Montage = Combo[ComboIndex].Montage;
	if (Montage == nullptr)
	{
		ComboReset();
		return;
	}

	RotateToCursor();

	bIsAttacking = true;
	bComboQueued = false;

	UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	if (Anim == nullptr) return;

	Anim->Montage_Play(Montage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AUT1Player::OnMontageEnd);
	Anim->Montage_SetEndDelegate(EndDelegate, Montage);
}

void AUT1Player::ComboReset()
{
	ComboIndex = 0;
	bIsAttacking = false;
	bComboQueued = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AUT1Player::OnMontageEnd(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted) return;   // 다른 몽타주에 밀린 경우는 무시

	if (bComboQueued)
	{
		ComboIndex++;
		PlayComboStep();
	}
	else
	{
		ComboReset();
	}
}

void AUT1Player::RotateToCursor()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		FHitResult Hit;
		if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			FVector Direction = Hit.ImpactPoint - GetActorLocation();
			Direction.Z = 0.0f;
			if (Direction.IsNearlyZero())
			{
				Direction = GetActorForwardVector();
			}

			SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
		}
	}
}

void AUT1Player::Input_Move(const FInputActionValue& InputValue)
{  
	if (bIsAttacking) return;

	FVector2D MovementVector = InputValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator CamYaw(0.f, SpringArm->GetComponentRotation().Yaw, 0.f);
		const FVector Forward = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::Y);

		AddMovementInput(Forward, MovementVector.Y);
		AddMovementInput(Right, MovementVector.X);
	}
}
