// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Crafting/UT1CraftingTypes.h"

#define LOCTEXT_NAMESPACE "UT1Crafting"

FText UT1Crafting::GetStatDisplayName(EUT1WeaponStat Stat)
{
	switch (Stat)
	{
	case EUT1WeaponStat::AttackPower: return LOCTEXT("Stat_AttackPower", "공격력");
	case EUT1WeaponStat::AttackSpeed: return LOCTEXT("Stat_AttackSpeed", "공격속도");
	case EUT1WeaponStat::AttackRange: return LOCTEXT("Stat_AttackRange", "공격 범위");
	case EUT1WeaponStat::CritChance:  return LOCTEXT("Stat_CritChance", "치명타");
	}
	return FText::GetEmpty();
}

FText UT1Crafting::GetResultMessage(EUT1WorkbenchResult Result)
{
	switch (Result)
	{
	case EUT1WorkbenchResult::Success:            return LOCTEXT("Result_Success", "완료");
	case EUT1WorkbenchResult::InvalidWeapon:      return LOCTEXT("Result_Invalid", "무기가 선택되지 않았습니다");
	case EUT1WorkbenchResult::NoBlueprint:        return LOCTEXT("Result_NoBlueprint", "설계도가 없습니다");
	case EUT1WorkbenchResult::AlreadyOwned:       return LOCTEXT("Result_AlreadyOwned", "이미 보유한 무기입니다");
	case EUT1WorkbenchResult::NotOwned:           return LOCTEXT("Result_NotOwned", "보유하지 않은 무기입니다");
	case EUT1WorkbenchResult::NotEnoughMaterials: return LOCTEXT("Result_NotEnough", "재료가 부족합니다");
	case EUT1WorkbenchResult::MaxLevel:           return LOCTEXT("Result_MaxLevel", "최대 레벨입니다");
	case EUT1WorkbenchResult::NoEnhanceRule:      return LOCTEXT("Result_NoRule", "강화 규칙이 설정되지 않았습니다");
	case EUT1WorkbenchResult::WeaponEquipped:     return LOCTEXT("Result_Equipped", "장착 중인 무기는 분해할 수 없습니다");
	}
	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
