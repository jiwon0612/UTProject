// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Crafting/UT1RunInventoryComponent.h"
#include "CJW/Crafting/UT1EnhancementRules.h"
#include "CJW/Crafting/UT1MaterialData.h"
#include "CJW/Weapons/UT1WeaponData.h"
#include "UT1.h"

namespace
{
	// 비용 목록을 재료별 합계로 접는다. 주재료와 보조재료가 같은 재료로
	// 지정된 경우 각각 따로 검사하면 실제로 필요한 양보다 적게 확인하게 된다.
	TMap<UUT1MaterialData*, int32> SumCosts(const TArray<FUT1MaterialCost>& Costs)
	{
		TMap<UUT1MaterialData*, int32> Sum;
		for (const FUT1MaterialCost& Cost : Costs)
		{
			if (Cost.Material != nullptr && Cost.Count > 0)
			{
				Sum.FindOrAdd(Cost.Material) += Cost.Count;
			}
		}
		return Sum;
	}
}

UUT1RunInventoryComponent::UUT1RunInventoryComponent()
{
	// 이벤트(획득/버튼)로만 바뀌는 데이터라 매 프레임 할 일이 없다.
	PrimaryComponentTick.bCanEverTick = false;
}

// ---------------------------------------------------------------- 재료

void UUT1RunInventoryComponent::AddMaterial(UUT1MaterialData* Material, int32 Count)
{
	if (Material == nullptr || Count <= 0)
	{
		return;
	}

	Materials.FindOrAdd(Material) += Count;
	OnInventoryChanged.Broadcast();
}

int32 UUT1RunInventoryComponent::GetMaterialCount(const UUT1MaterialData* Material) const
{
	// 키 타입이 TObjectPtr<UUT1MaterialData>(비 const)라 조회용으로만 const 를 벗긴다.
	const int32* Count = Materials.Find(const_cast<UUT1MaterialData*>(Material));
	return Count != nullptr ? *Count : 0;
}

bool UUT1RunInventoryComponent::HasMaterials(const TArray<FUT1MaterialCost>& Costs) const
{
	for (const TPair<UUT1MaterialData*, int32>& Need : SumCosts(Costs))
	{
		if (GetMaterialCount(Need.Key) < Need.Value)
		{
			return false;
		}
	}
	return true;
}

void UUT1RunInventoryComponent::SpendMaterials(const TArray<FUT1MaterialCost>& Costs, FUT1OwnedWeapon* RecordTo)
{
	for (const TPair<UUT1MaterialData*, int32>& Need : SumCosts(Costs))
	{
		int32& Have = Materials.FindOrAdd(Need.Key);
		Have -= Need.Value;
		ensureMsgf(Have >= 0, TEXT("SpendMaterials 는 HasMaterials 검사 뒤에만 불러야 합니다."));

		// 0 개가 된 재료는 키를 지운다. 남겨 두면 UI 목록에 "x0" 이 계속 보인다.
		if (Have <= 0)
		{
			Materials.Remove(Need.Key);
		}

		if (RecordTo != nullptr)
		{
			RecordTo->SpentMaterials.FindOrAdd(Need.Key) += Need.Value;
		}
	}
}

// ---------------------------------------------------------------- 설계도

bool UUT1RunInventoryComponent::UnlockBlueprint(UUT1WeaponData* Weapon)
{
	if (Weapon == nullptr || HasBlueprint(Weapon))
	{
		return false;
	}

	UnlockedBlueprints.Add(Weapon);
	OnInventoryChanged.Broadcast();
	return true;
}

bool UUT1RunInventoryComponent::HasBlueprint(const UUT1WeaponData* Weapon) const
{
	return Weapon != nullptr && UnlockedBlueprints.Contains(Weapon);
}

// ---------------------------------------------------------------- 제작

EUT1WorkbenchResult UUT1RunInventoryComponent::CanCraft(const UUT1WeaponData* Weapon) const
{
	if (Weapon == nullptr)
	{
		return EUT1WorkbenchResult::InvalidWeapon;
	}
	if (HasBlueprint(Weapon) == false)
	{
		return EUT1WorkbenchResult::NoBlueprint;
	}
	if (IsWeaponOwned(Weapon))
	{
		return EUT1WorkbenchResult::AlreadyOwned;
	}
	if (HasMaterials(Weapon->CraftCost) == false)
	{
		return EUT1WorkbenchResult::NotEnoughMaterials;
	}
	return EUT1WorkbenchResult::Success;
}

EUT1WorkbenchResult UUT1RunInventoryComponent::Craft(UUT1WeaponData* Weapon)
{
	// 검사와 실행이 같은 함수(CanCraft)를 거치게 해서, UI 에 보이는
	// "만들 수 있음" 과 실제 결과가 어긋나지 않게 한다.
	const EUT1WorkbenchResult Result = CanCraft(Weapon);
	if (Result != EUT1WorkbenchResult::Success)
	{
		return Result;
	}

	// 제작 비용은 기록하지 않는다. 분해 환급은 강화에 쓴 재료만 대상이다.
	SpendMaterials(Weapon->CraftCost, nullptr);

	FUT1OwnedWeapon& NewWeapon = OwnedWeapons.AddDefaulted_GetRef();
	NewWeapon.WeaponData = Weapon;

	OnInventoryChanged.Broadcast();
	return EUT1WorkbenchResult::Success;
}

bool UUT1RunInventoryComponent::AddOwnedWeapon(UUT1WeaponData* Weapon)
{
	if (Weapon == nullptr || IsWeaponOwned(Weapon))
	{
		return false;
	}

	FUT1OwnedWeapon& NewWeapon = OwnedWeapons.AddDefaulted_GetRef();
	NewWeapon.WeaponData = Weapon;

	OnInventoryChanged.Broadcast();
	return true;
}

bool UUT1RunInventoryComponent::IsWeaponOwned(const UUT1WeaponData* Weapon) const
{
	return FindOwned(Weapon) != nullptr;
}

// ---------------------------------------------------------------- 강화

int32 UUT1RunInventoryComponent::GetStatLevel(const UUT1WeaponData* Weapon, EUT1WeaponStat Stat) const
{
	const FUT1OwnedWeapon* Owned = FindOwned(Weapon);
	return Owned != nullptr ? Owned->GetStatLevel(Stat) : 0;
}

bool UUT1RunInventoryComponent::GetEnhanceCost(const UUT1WeaponData* Weapon, EUT1WeaponStat Stat, TArray<FUT1MaterialCost>& OutCosts) const
{
	OutCosts.Reset();

	const FUT1OwnedWeapon* Owned = FindOwned(Weapon);
	if (Owned == nullptr || EnhancementRules == nullptr)
	{
		return false;
	}

	return EnhancementRules->GetEnhanceCost(Stat, Owned->GetStatLevel(Stat), OutCosts);
}

EUT1WorkbenchResult UUT1RunInventoryComponent::CanEnhance(const UUT1WeaponData* Weapon, EUT1WeaponStat Stat) const
{
	if (Weapon == nullptr)
	{
		return EUT1WorkbenchResult::InvalidWeapon;
	}

	const FUT1OwnedWeapon* Owned = FindOwned(Weapon);
	if (Owned == nullptr)
	{
		return EUT1WorkbenchResult::NotOwned;
	}

	const FUT1StatEnhanceRule* Rule = EnhancementRules != nullptr ? EnhancementRules->FindRule(Stat) : nullptr;
	if (Rule == nullptr)
	{
		return EUT1WorkbenchResult::NoEnhanceRule;
	}

	if (Owned->GetStatLevel(Stat) >= Rule->MaxLevel)
	{
		return EUT1WorkbenchResult::MaxLevel;
	}

	TArray<FUT1MaterialCost> Costs;
	GetEnhanceCost(Weapon, Stat, Costs);
	if (HasMaterials(Costs) == false)
	{
		return EUT1WorkbenchResult::NotEnoughMaterials;
	}

	return EUT1WorkbenchResult::Success;
}

EUT1WorkbenchResult UUT1RunInventoryComponent::Enhance(UUT1WeaponData* Weapon, EUT1WeaponStat Stat)
{
	const EUT1WorkbenchResult Result = CanEnhance(Weapon, Stat);
	if (Result != EUT1WorkbenchResult::Success)
	{
		return Result;
	}

	FUT1OwnedWeapon* Owned = FindOwned(Weapon);
	check(Owned != nullptr);   // CanEnhance 가 보장한다.

	TArray<FUT1MaterialCost> Costs;
	GetEnhanceCost(Weapon, Stat, Costs);
	SpendMaterials(Costs, Owned);

	Owned->StatLevels.FindOrAdd(Stat) += 1;

	// 장착 중인 무기를 강화해도 다시 장착할 필요는 없다. 전투 스탯은
	// 공격할 때마다 이 레벨을 읽어서 계산하기 때문이다 (2단계).
	OnInventoryChanged.Broadcast();
	return EUT1WorkbenchResult::Success;
}

// ---------------------------------------------------------------- 분해

EUT1WorkbenchResult UUT1RunInventoryComponent::CanDismantle(const UUT1WeaponData* Weapon) const
{
	if (Weapon == nullptr)
	{
		return EUT1WorkbenchResult::InvalidWeapon;
	}
	if (IsWeaponOwned(Weapon) == false)
	{
		return EUT1WorkbenchResult::NotOwned;
	}
	// 들고 있는 무기를 분해하면 맨손이 된다. 맨손 공격이 없으므로 막는다.
	if (EquippedWeapon == Weapon)
	{
		return EUT1WorkbenchResult::WeaponEquipped;
	}
	return EUT1WorkbenchResult::Success;
}

void UUT1RunInventoryComponent::GetDismantleRefund(const UUT1WeaponData* Weapon, TArray<FUT1MaterialCost>& OutRefund) const
{
	OutRefund.Reset();

	const FUT1OwnedWeapon* Owned = FindOwned(Weapon);
	if (Owned == nullptr)
	{
		return;
	}

	const float Rate = EnhancementRules != nullptr ? EnhancementRules->DismantleRefundRate : 0.0f;

	for (const TPair<TObjectPtr<UUT1MaterialData>, int32>& Spent : Owned->SpentMaterials)
	{
		// 내림. 1 개를 쓰고 70% 를 돌려받으면 0 개다. 손해가 확실히 보이게 하려는 의도.
		const int32 Count = FMath::FloorToInt(Spent.Value * Rate);
		if (Count > 0)
		{
			FUT1MaterialCost& Refund = OutRefund.AddDefaulted_GetRef();
			Refund.Material = Spent.Key;
			Refund.Count = Count;
		}
	}
}

EUT1WorkbenchResult UUT1RunInventoryComponent::Dismantle(UUT1WeaponData* Weapon)
{
	const EUT1WorkbenchResult Result = CanDismantle(Weapon);
	if (Result != EUT1WorkbenchResult::Success)
	{
		return Result;
	}

	// 무기를 지우기 전에 환급량을 계산한다. 지운 뒤에는 SpentMaterials 가 없다.
	TArray<FUT1MaterialCost> Refund;
	GetDismantleRefund(Weapon, Refund);

	for (const FUT1MaterialCost& Cost : Refund)
	{
		Materials.FindOrAdd(Cost.Material) += Cost.Count;
	}

	OwnedWeapons.RemoveAll([Weapon](const FUT1OwnedWeapon& Owned)
	{
		return Owned.WeaponData == Weapon;
	});

	// AddMaterial 을 쓰지 않은 이유: 재료마다 알림이 나가면 UI 가 여러 번 다시 그려진다.
	OnInventoryChanged.Broadcast();
	return EUT1WorkbenchResult::Success;
}

// ---------------------------------------------------------------- 장착

EUT1WorkbenchResult UUT1RunInventoryComponent::EquipWeapon(UUT1WeaponData* Weapon)
{
	if (Weapon == nullptr)
	{
		return EUT1WorkbenchResult::InvalidWeapon;
	}
	if (IsWeaponOwned(Weapon) == false)
	{
		return EUT1WorkbenchResult::NotOwned;
	}
	if (EquippedWeapon == Weapon)
	{
		return EUT1WorkbenchResult::Success;   // 이미 들고 있다. 액터를 다시 만들 필요 없음.
	}

	EquippedWeapon = Weapon;

	OnEquippedWeaponChanged.Broadcast(EquippedWeapon);
	OnInventoryChanged.Broadcast();
	return EUT1WorkbenchResult::Success;
}

const FUT1OwnedWeapon* UUT1RunInventoryComponent::GetEquippedOwnedWeapon() const
{
	return FindOwned(EquippedWeapon);
}

// ---------------------------------------------------------------- 전투 스탯

int32 UUT1RunInventoryComponent::GetTotalEnhanceLevel(const UUT1WeaponData* Weapon) const
{
	const FUT1OwnedWeapon* Owned = FindOwned(Weapon);
	if (Owned == nullptr)
	{
		return 0;
	}

	int32 Total = 0;
	for (const TPair<EUT1WeaponStat, int32>& Pair : Owned->StatLevels)
	{
		Total += Pair.Value;
	}
	return Total;
}

bool UUT1RunInventoryComponent::ShouldShowAura(const UUT1WeaponData* Weapon) const
{
	return EnhancementRules != nullptr
		&& GetTotalEnhanceLevel(Weapon) >= EnhancementRules->AuraEnhanceLevelThreshold;
}

float UUT1RunInventoryComponent::GetStatBonus(const UUT1WeaponData* Weapon, EUT1WeaponStat Stat) const
{
	if (EnhancementRules == nullptr)
	{
		return 0.0f;
	}
	return EnhancementRules->GetTotalBonus(Stat, GetStatLevel(Weapon, Stat));
}

FUT1WeaponCombatStats UUT1RunInventoryComponent::GetCombatStats(const UUT1WeaponData* Weapon) const
{
	FUT1WeaponCombatStats Stats;
	if (Weapon == nullptr)
	{
		return Stats;
	}

	// 공격력/공속/범위는 기본값에 비율로 곱하고, 치명타는 확률에 더한다.
	// 비율로 두면 무기 기본값이 달라도 강화 1레벨의 "체감 비율"이 같다.
	Stats.DamageMultiplier = 1.0f + GetStatBonus(Weapon, EUT1WeaponStat::AttackPower);
	Stats.AttackSpeed = Weapon->BaseAttackSpeed * (1.0f + GetStatBonus(Weapon, EUT1WeaponStat::AttackSpeed));
	Stats.RangeScale = 1.0f + GetStatBonus(Weapon, EUT1WeaponStat::AttackRange);
	Stats.CritChance = FMath::Clamp(Weapon->BaseCritChance + GetStatBonus(Weapon, EUT1WeaponStat::CritChance), 0.0f, 1.0f);

	if (EnhancementRules != nullptr)
	{
		Stats.CritDamageMultiplier = EnhancementRules->CritDamageMultiplier;
	}
	return Stats;
}

// ---------------------------------------------------------------- 내부

FUT1OwnedWeapon* UUT1RunInventoryComponent::FindOwned(const UUT1WeaponData* Weapon)
{
	if (Weapon == nullptr)
	{
		return nullptr;
	}

	return OwnedWeapons.FindByPredicate([Weapon](const FUT1OwnedWeapon& Owned)
	{
		return Owned.WeaponData == Weapon;
	});
}

const FUT1OwnedWeapon* UUT1RunInventoryComponent::FindOwned(const UUT1WeaponData* Weapon) const
{
	return const_cast<UUT1RunInventoryComponent*>(this)->FindOwned(Weapon);
}
