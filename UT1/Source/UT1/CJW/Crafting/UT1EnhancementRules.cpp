// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Crafting/UT1EnhancementRules.h"

const FUT1StatEnhanceRule* UUT1EnhancementRules::FindRule(EUT1WeaponStat Stat) const
{
	return StatRules.FindByPredicate([Stat](const FUT1StatEnhanceRule& Rule)
	{
		return Rule.Stat == Stat;
	});
}

float UUT1EnhancementRules::GetTotalBonus(EUT1WeaponStat Stat, int32 Level) const
{
	const FUT1StatEnhanceRule* Rule = FindRule(Stat);
	return Rule != nullptr ? Rule->BonusPerLevel * FMath::Max(Level, 0) : 0.0f;
}

bool UUT1EnhancementRules::GetEnhanceCost(EUT1WeaponStat Stat, int32 CurrentLevel, TArray<FUT1MaterialCost>& OutCosts) const
{
	OutCosts.Reset();

	const FUT1StatEnhanceRule* Rule = FindRule(Stat);
	if (Rule == nullptr)
	{
		return false;
	}

	const int32 Increase = Rule->CostIncreasePerLevel * FMath::Max(CurrentLevel, 0);

	// 비용이 0 이 된 재료는 목록에서 뺀다. UI 에 "고철 x0" 이 뜨지 않게 하기 위해서다.
	auto AddCost = [&OutCosts](UUT1MaterialData* Material, int32 Count)
	{
		if (Material != nullptr && Count > 0)
		{
			FUT1MaterialCost& Cost = OutCosts.AddDefaulted_GetRef();
			Cost.Material = Material;
			Cost.Count = Count;
		}
	};

	// 기본 비용이 0 인 재료는 일부러 뺀 것이므로 레벨 증가분도 붙이지 않는다.
	AddCost(Rule->MainMaterial, Rule->MainCost > 0 ? Rule->MainCost + Increase : 0);
	AddCost(Rule->SubMaterial, Rule->SubCost > 0 ? Rule->SubCost + Increase : 0);
	return true;
}
