// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "UT1AnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class UT1_API UUT1AnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UUT1AnimInstance();

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "References")
	TObjectPtr<class ACharacter> Character;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "References")
	TObjectPtr<class UCharacterMovementComponent> MovementComponent;

protected:
	UPROPERTY(BlueprintReadWrite, Category = "Movement Data")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Movement Data")
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Movement Data")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadWrite, Category = "Movement Data")
	bool bIsFalling = false;

};
