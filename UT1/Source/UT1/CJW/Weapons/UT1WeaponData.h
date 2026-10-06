// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
#include "CJW/Crafting/UT1CraftingTypes.h"
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
	// WeaponName 이 비어 있으면 에셋 이름을 쓴다.
	UFUNCTION(BlueprintPure, Category = "Info")
	FText GetDisplayText() const
	{
		return WeaponName.IsNone() ? FText::FromString(GetName()) : FText::FromName(WeaponName);
	}

	UPROPERTY(EditAnywhere, Category = "Info")
	FName WeaponName;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TArray<FComboStep> ComboSequence;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float BaseDamage = 10.f;

	// 콤보 몽타주 재생 속도의 기준값. 강화 배율이 여기에 곱해진다.
	// 무기 수치를 비슷하게 맞추기로 했으므로 보통 1.0 으로 둔다.
	UPROPERTY(EditAnywhere, Category = "Combat", meta = (ClampMin = "0.1"))
	float BaseAttackSpeed = 1.0f;

	// 강화 전 치명타 확률 (0~1).
	UPROPERTY(EditAnywhere, Category = "Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseCritChance = 0.05f;

	UPROPERTY(EditAnywhere, Category = "EquipMesh")
	TArray<FEquipmentData> EquipMeshes;

	// 설계도를 가진 상태에서 이 재료를 내면 제작된다.
	// 설계도와 무기가 1:1 이라 레시피 에셋을 따로 두지 않고 무기에 붙였다.
	// 이 에셋은 모든 런이 공유하는 "정의"이므로 강화 레벨처럼 런 중에
	// 바뀌는 값은 여기에 두지 않는다 (FUT1OwnedWeapon 참고).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	TArray<FUT1MaterialCost> CraftCost;
};
