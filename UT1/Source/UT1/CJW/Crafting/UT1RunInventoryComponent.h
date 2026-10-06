// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CJW/Crafting/UT1CraftingTypes.h"
#include "UT1RunInventoryComponent.generated.h"

class UUT1MaterialData;
class UUT1WeaponData;
class UUT1EnhancementRules;

/**
 * 이번 런에서 플레이어가 가진 무기 하나.
 *
 * UUT1WeaponData 는 에디터 에셋이라 모든 곳이 같은 객체를 공유한다.
 * 거기에 강화 레벨을 쓰면 에셋 원본이 바뀌어 버리므로, 런 중에 변하는 값은
 * 이 구조체에 따로 둔다. (정의 Definition / 인스턴스 Instance 분리)
 */
USTRUCT(BlueprintType)
struct FUT1OwnedWeapon
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UUT1WeaponData> WeaponData;

	// 강화하지 않은 스탯은 키가 없다. 읽을 때는 GetStatLevel 을 쓴다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<EUT1WeaponStat, int32> StatLevels;

	// 분해 환급의 기준. 규칙에서 역산하지 않고 실제로 낸 양을 기록해 두면
	// 런 도중 규칙(비용)이 바뀌어도 환급이 어긋나지 않는다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<TObjectPtr<UUT1MaterialData>, int32> SpentMaterials;

	int32 GetStatLevel(EUT1WeaponStat Stat) const
	{
		const int32* Level = StatLevels.Find(Stat);
		return Level != nullptr ? *Level : 0;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUT1RunInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUT1EquippedWeaponChanged, UUT1WeaponData*, NewWeapon);

/**
 * 한 런 동안 쌓이는 것들: 재료, 해금한 설계도, 보유 무기와 강화 상태, 장착 무기.
 *
 * 재료와 무기를 한 컴포넌트에 둔 이유는 수명이 같기 때문이다. 둘 다 "이번 런"에만
 * 유효하고 플레이어가 죽으면 함께 사라진다. 플레이어에 붙어 있으므로 런 초기화를
 * 위한 코드가 따로 필요 없다. 메타 진행(런 간 유지)을 넣게 되면 이 데이터를
 * GameInstance 나 SaveGame 으로 옮기는 것이 다음 단계다.
 *
 * 이 컴포넌트는 규칙만 판단하고 화면이나 메시를 모른다. 장착이 바뀌면
 * OnEquippedWeaponChanged 만 알리고, 실제 무기 액터 교체는 소유자(플레이어)가 한다.
 * UI 도 OnInventoryChanged 를 구독해서 갱신한다.
 *
 * 무기는 중복 보유가 불가능하므로 UUT1WeaponData 포인터 자체를 키로 쓴다.
 * 배열 인덱스와 달리 분해로 목록이 줄어도 가리키는 대상이 바뀌지 않는다.
 */
UCLASS(ClassGroup = (UT1), meta = (BlueprintSpawnableComponent))
class UT1_API UUT1RunInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUT1RunInventoryComponent();

	// --- 재료 ---
	UFUNCTION(BlueprintCallable, Category = "RunInventory|Material")
	void AddMaterial(UUT1MaterialData* Material, int32 Count);

	UFUNCTION(BlueprintPure, Category = "RunInventory|Material")
	int32 GetMaterialCount(const UUT1MaterialData* Material) const;

	// 같은 재료가 목록에 두 번 있어도 합산해서 검사한다.
	UFUNCTION(BlueprintPure, Category = "RunInventory|Material")
	bool HasMaterials(const TArray<FUT1MaterialCost>& Costs) const;

	// --- 설계도 ---
	// 새로 해금했으면 true. 이미 가진 설계도면 false.
	UFUNCTION(BlueprintCallable, Category = "RunInventory|Blueprint")
	bool UnlockBlueprint(UUT1WeaponData* Weapon);

	UFUNCTION(BlueprintPure, Category = "RunInventory|Blueprint")
	bool HasBlueprint(const UUT1WeaponData* Weapon) const;

	// --- 제작 ---
	UFUNCTION(BlueprintPure, Category = "RunInventory|Craft")
	EUT1WorkbenchResult CanCraft(const UUT1WeaponData* Weapon) const;

	UFUNCTION(BlueprintCallable, Category = "RunInventory|Craft")
	EUT1WorkbenchResult Craft(UUT1WeaponData* Weapon);

	// 비용 없이 보관함에 넣는다. 시작 무기나 보상 지급용.
	UFUNCTION(BlueprintCallable, Category = "RunInventory|Craft")
	bool AddOwnedWeapon(UUT1WeaponData* Weapon);

	UFUNCTION(BlueprintPure, Category = "RunInventory|Craft")
	bool IsWeaponOwned(const UUT1WeaponData* Weapon) const;

	// --- 강화 ---
	UFUNCTION(BlueprintPure, Category = "RunInventory|Enhance")
	int32 GetStatLevel(const UUT1WeaponData* Weapon, EUT1WeaponStat Stat) const;

	// 다음 레벨로 올리는 비용. 규칙이 없거나 보유하지 않은 무기면 false.
	UFUNCTION(BlueprintPure, Category = "RunInventory|Enhance")
	bool GetEnhanceCost(const UUT1WeaponData* Weapon, EUT1WeaponStat Stat, TArray<FUT1MaterialCost>& OutCosts) const;

	UFUNCTION(BlueprintPure, Category = "RunInventory|Enhance")
	EUT1WorkbenchResult CanEnhance(const UUT1WeaponData* Weapon, EUT1WeaponStat Stat) const;

	UFUNCTION(BlueprintCallable, Category = "RunInventory|Enhance")
	EUT1WorkbenchResult Enhance(UUT1WeaponData* Weapon, EUT1WeaponStat Stat);

	// --- 분해 ---
	UFUNCTION(BlueprintPure, Category = "RunInventory|Dismantle")
	EUT1WorkbenchResult CanDismantle(const UUT1WeaponData* Weapon) const;

	UFUNCTION(BlueprintPure, Category = "RunInventory|Dismantle")
	void GetDismantleRefund(const UUT1WeaponData* Weapon, TArray<FUT1MaterialCost>& OutRefund) const;

	// 무기를 보관함에서 지우고 강화 재료 일부를 돌려준다. 설계도는 남는다.
	UFUNCTION(BlueprintCallable, Category = "RunInventory|Dismantle")
	EUT1WorkbenchResult Dismantle(UUT1WeaponData* Weapon);

	// --- 장착 ---
	// 언제 바꿀 수 있는지(작업대에서만)는 부르는 쪽의 책임이다.
	// 이 컴포넌트는 "가진 무기인가"만 본다.
	UFUNCTION(BlueprintCallable, Category = "RunInventory|Equip")
	EUT1WorkbenchResult EquipWeapon(UUT1WeaponData* Weapon);

	UFUNCTION(BlueprintPure, Category = "RunInventory|Equip")
	UUT1WeaponData* GetEquippedWeapon() const { return EquippedWeapon; }

	// 장착 무기의 강화 상태. 맨손이면 nullptr.
	const FUT1OwnedWeapon* GetEquippedOwnedWeapon() const;

	// --- 전투 스탯 ---
	// 무기 기본값 + 강화 레벨 + 강화 규칙으로 최종 수치를 만든다. 계산은 여기 한 곳뿐이다.
	// 보유하지 않은 무기는 강화 0 으로 계산한다.
	UFUNCTION(BlueprintPure, Category = "RunInventory|Combat")
	FUT1WeaponCombatStats GetCombatStats(const UUT1WeaponData* Weapon) const;

	UFUNCTION(BlueprintPure, Category = "RunInventory|Combat")
	FUT1WeaponCombatStats GetEquippedCombatStats() const { return GetCombatStats(EquippedWeapon); }

	// 해당 스탯의 강화로 붙은 보너스 (0.2 = +20%, 치명타는 +20%p). UI 표시용.
	UFUNCTION(BlueprintPure, Category = "RunInventory|Combat")
	float GetStatBonus(const UUT1WeaponData* Weapon, EUT1WeaponStat Stat) const;

	// --- 조회 (C++ 용. 블루프린트는 아래 UPROPERTY 로 읽는다) ---
	const TMap<TObjectPtr<UUT1MaterialData>, int32>& GetMaterials() const { return Materials; }
	const TArray<TObjectPtr<UUT1WeaponData>>& GetUnlockedBlueprints() const { return UnlockedBlueprints; }
	const TArray<FUT1OwnedWeapon>& GetOwnedWeapons() const { return OwnedWeapons; }
	const UUT1EnhancementRules* GetEnhancementRules() const { return EnhancementRules; }

public:
	// 재료, 설계도, 무기, 강화 중 무엇이든 바뀌면 한 번 알린다.
	// UI 는 이걸 받으면 화면 전체를 다시 그리면 된다. 지금 규모에서는
	// 변경 종류별 델리게이트보다 이쪽이 단순하고 실수할 곳이 적다.
	UPROPERTY(BlueprintAssignable, Category = "RunInventory")
	FUT1RunInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "RunInventory")
	FUT1EquippedWeaponChanged OnEquippedWeaponChanged;

protected:
	// 게임 전체에 하나(DA_EnhancementRules). BP_Player 에서 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RunInventory")
	TObjectPtr<UUT1EnhancementRules> EnhancementRules;

private:
	FUT1OwnedWeapon* FindOwned(const UUT1WeaponData* Weapon);
	const FUT1OwnedWeapon* FindOwned(const UUT1WeaponData* Weapon) const;

	// 재료를 빼고, RecordTo 가 있으면 그 무기의 SpentMaterials 에 기록한다.
	// HasMaterials 로 검사가 끝난 뒤에만 부른다.
	void SpendMaterials(const TArray<FUT1MaterialCost>& Costs, FUT1OwnedWeapon* RecordTo);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RunInventory", meta = (AllowPrivateAccess = "true"))
	TMap<TObjectPtr<UUT1MaterialData>, int32> Materials;

	// 순서가 UI 목록 순서가 되므로 TSet 이 아니라 배열로 둔다 (얻은 순서대로).
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RunInventory", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UUT1WeaponData>> UnlockedBlueprints;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RunInventory", meta = (AllowPrivateAccess = "true"))
	TArray<FUT1OwnedWeapon> OwnedWeapons;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RunInventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUT1WeaponData> EquippedWeapon;
};
