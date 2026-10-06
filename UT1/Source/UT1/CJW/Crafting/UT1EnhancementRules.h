// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CJW/Crafting/UT1CraftingTypes.h"
#include "UT1EnhancementRules.generated.h"

/**
 * 스탯 하나의 강화 규칙.
 *
 * 재료는 3종인데 스탯은 4개라서, 스탯마다 "주재료 + 보조재료" 비율로
 * 구분한다. 예) 공격력 = 고철 3 + 전선 1, 치명타 = 고철 1 + 천 3.
 */
USTRUCT(BlueprintType)
struct FUT1StatEnhanceRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUT1WeaponStat Stat = EUT1WeaponStat::AttackPower;

	// 레벨당 증가량. 의미는 스탯마다 다르다.
	// 공격력 / 공격속도 / 공격 범위: 기본값에 곱하는 비율 (0.1 = 레벨당 +10%)
	// 치명타: 확률에 더하는 값 (0.03 = 레벨당 +3%p)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float BonusPerLevel = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 MaxLevel = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	TObjectPtr<UUT1MaterialData> MainMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0"))
	int32 MainCost = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	TObjectPtr<UUT1MaterialData> SubMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0"))
	int32 SubCost = 1;

	// 현재 레벨마다 주재료/보조재료 비용에 더해지는 값.
	// 0 이면 레벨과 상관없이 비용이 같다. 밸런스 조절용 손잡이.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0"))
	int32 CostIncreasePerLevel = 0;
};

/**
 * 강화와 분해 규칙 전체. 게임에 하나만 만들어 두고(DA_EnhancementRules)
 * 플레이어의 런 인벤토리 컴포넌트가 참조한다.
 *
 * 규칙을 무기마다 두지 않은 이유: 무기 기본 수치를 비슷하게 맞추기로 했으므로
 * 강화 규칙도 공통으로 두는 편이 밸런스를 한 곳에서 관리하기 쉽다.
 * 무기별 차이가 필요해지면 UUT1WeaponData 에 덮어쓰기용 규칙을 추가하면 된다.
 */
UCLASS(BlueprintType)
class UT1_API UUT1EnhancementRules : public UDataAsset
{
	GENERATED_BODY()

public:
	const FUT1StatEnhanceRule* FindRule(EUT1WeaponStat Stat) const;

	// 해당 레벨까지의 누적 보너스 (레벨 x 레벨당 증가량). 규칙이 없으면 0.
	float GetTotalBonus(EUT1WeaponStat Stat, int32 Level) const;

	// CurrentLevel 에서 한 단계 올릴 때의 비용. 규칙이 없으면 false.
	// 최대 레벨 검사는 하지 않는다. 그건 호출하는 쪽이 이유를 구분해서 알려야 하기 때문이다.
	bool GetEnhanceCost(EUT1WeaponStat Stat, int32 CurrentLevel, TArray<FUT1MaterialCost>& OutCosts) const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhance", meta = (TitleProperty = "Stat"))
	TArray<FUT1StatEnhanceRule> StatRules;

	// 분해할 때 강화에 쓴 재료를 얼마나 돌려줄지. 손해를 남겨 두어야
	// 무기를 갈아타는 선택에 무게가 생긴다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismantle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DismantleRefundRate = 0.7f;

	// 무기 하나의 강화 레벨 합계(4개 스탯)가 이 값 이상이면 무기에 오라 이펙트가 켜진다.
	// "많이 키운 무기"를 눈으로 보여 주는 보상이라, 밸런스와 함께 여기서 조절한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura", meta = (ClampMin = "1"))
	int32 AuraEnhanceLevelThreshold = 10;

	// 치명타가 터졌을 때 데미지 배율. 강화는 확률만 올리고 배율은 고정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhance", meta = (ClampMin = "1.0"))
	float CritDamageMultiplier = 1.5f;
};
