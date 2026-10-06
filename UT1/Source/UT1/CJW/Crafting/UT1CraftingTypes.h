// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UT1CraftingTypes.generated.h"

class UUT1MaterialData;

/**
 * 강화할 수 있는 무기 스탯.
 * 강화 규칙(UUT1EnhancementRules)과 보유 무기의 레벨(FUT1OwnedWeapon)이
 * 같은 키를 쓰도록 enum 하나로 묶어 둔다.
 */
UENUM(BlueprintType)
enum class EUT1WeaponStat : uint8
{
	AttackPower UMETA(DisplayName = "공격력"),
	AttackSpeed UMETA(DisplayName = "공격속도"),
	AttackRange UMETA(DisplayName = "공격 범위"),
	CritChance  UMETA(DisplayName = "치명타"),
};

/**
 * 작업대 요청(제작/강화/분해/장착)의 결과.
 * bool 대신 이유까지 돌려줘서 UI 가 "재료 부족", "최대 레벨" 같은
 * 안내를 로직을 다시 검사하지 않고 그대로 띄울 수 있게 한다.
 */
UENUM(BlueprintType)
enum class EUT1WorkbenchResult : uint8
{
	Success,
	InvalidWeapon,
	NoBlueprint,
	AlreadyOwned,
	NotOwned,
	NotEnoughMaterials,
	MaxLevel,
	NoEnhanceRule,
	WeaponEquipped,
};

namespace UT1Crafting
{
	// 화면에 보여줄 문구. UMETA(DisplayName) 은 에디터 전용 메타데이터라
	// 패키징 빌드에서는 사라지므로 표시용 텍스트는 코드로 따로 둔다.
	UT1_API FText GetStatDisplayName(EUT1WeaponStat Stat);
	UT1_API FText GetResultMessage(EUT1WorkbenchResult Result);

	// UI 가 스탯을 순서대로 그릴 때 쓴다.
	inline constexpr EUT1WeaponStat AllWeaponStats[] =
	{
		EUT1WeaponStat::AttackPower,
		EUT1WeaponStat::AttackSpeed,
		EUT1WeaponStat::AttackRange,
		EUT1WeaponStat::CritChance,
	};
}

/**
 * 강화가 반영된 최종 전투 수치. UUT1RunInventoryComponent::GetCombatStats 한 곳에서만 계산한다.
 * 전투(데미지, 몽타주 속도, 무기 크기, 치명타)와 UI 가 같은 숫자를 보게 하기 위해서다.
 */
USTRUCT(BlueprintType)
struct FUT1WeaponCombatStats
{
	GENERATED_BODY()

	// 기본 데미지 x 콤보 배율에 곱한다.
	UPROPERTY(BlueprintReadOnly)
	float DamageMultiplier = 1.0f;

	// 몽타주 PlayRate. 무기 기본 공속까지 포함된 최종값.
	UPROPERTY(BlueprintReadOnly)
	float AttackSpeed = 1.0f;

	// 무기 액터 크기. 판정 점이 무기의 자식이라 리치도 같이 늘어난다.
	UPROPERTY(BlueprintReadOnly)
	float RangeScale = 1.0f;

	// 0~1. 무기 기본 치명타 + 강화.
	UPROPERTY(BlueprintReadOnly)
	float CritChance = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float CritDamageMultiplier = 1.5f;
};

/** 재료 한 종류와 개수. 제작 비용과 강화 비용이 같이 쓴다. */
USTRUCT(BlueprintType)
struct FUT1MaterialCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UUT1MaterialData> Material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 Count = 1;
};
