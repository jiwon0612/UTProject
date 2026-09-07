// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Player/UT1Player.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

void AUT1Player::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
}

void AUT1Player::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this,&AUT1Player::Input_Move);
	}
}

void AUT1Player::Input_Move(const FInputActionValue& InputValue)
{  
	FVector2D MovementVector = InputValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator CamYaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Forward = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::Y);

		AddMovementInput(Forward, MovementVector.Y);
		AddMovementInput(Right, MovementVector.X);
	}
}
