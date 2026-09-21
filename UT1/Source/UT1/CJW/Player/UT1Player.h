// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CJW/Entities/UT1Entity.h"
#include "UT1Player.generated.h"

struct FInputActionValue;

/**
 * 
 */
UCLASS()
class UT1_API AUT1Player : public AUT1Entity
{
	GENERATED_BODY()
public:
	AUT1Player();
public:
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	void ComboAttack();

	void PlayComboStep();
	void ComboReset();

	int32 ComboIndex = 0;
	bool bIsAttacking = false;
	bool bComboQueued = false;

	UFUNCTION()
	void OnMontageEnd(UAnimMontage* Montage, bool bInterrupted);
	
	void RotateToCursor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> Camera;

public:
	void Input_Move(const FInputActionValue& InputValue);

protected:
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> AttackAction;

protected:
	UPROPERTY(EditAnywhere, Category = Test)
	TObjectPtr<class UUT1WeaponData> TestWeaponData;
};
