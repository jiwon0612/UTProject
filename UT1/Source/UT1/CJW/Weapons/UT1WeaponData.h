// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
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

USTRUCT(BlueprintType)
struct FEquipmentData
{
	GENERATED_BODY()

	// 무기 메시가 아니라 무기 BP 클래스를 가리킨다.
	// 손잡이 정렬은 그 BP 안에서 끝내므로 여기서는 보정값이 필요 없다.
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AUT1Weapon> WeaponClass;

	UPROPERTY(EditAnywhere)
	bool bIsRightHanded = true;
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

	UPROPERTY(EditAnywhere, Category = "EquipMesh")
	TArray<FEquipmentData> EquipMeshes;
};
