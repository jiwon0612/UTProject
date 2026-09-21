// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UT1WeaponData.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FComboStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UAnimMontage> Montage;

	UPROPERTY(EditAnywhere)
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere)
	float ComboWindowStart = 0.4f;

	UPROPERTY(EditAnywhere)
	float ComboWindowEnd = 0.8f;
};

UCLASS()
class UT1_API UUT1WeaponData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Info")
	FName WeaponName;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TArray<FComboStep> ComboSequence;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float BaseDamage = 10.f;
};
