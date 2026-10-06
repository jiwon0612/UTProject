// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CJW/Crafting/UT1CraftingTypes.h"
#include "UT1WorkbenchStatRow.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUT1EnhanceRequested, EUT1WeaponStat, Stat);

/**
 * 작업대 오른쪽 상세의 강화 스탯 한 줄.
 *
 * 레이아웃: WBP 로 상속해 같은 이름의 위젯을 배치하면 그 레이아웃을 쓰고,
 * 비어 있으면 C++ 이 기본 레이아웃을 만든다.
 *   HorizontalBox [ StatNameText | LevelText | CostText | EnhanceButton ]
 */
UCLASS()
class UT1_API UUT1WorkbenchStatRow : public UUserWidget
{
	GENERATED_BODY()

public:
	// BonusText 는 현재 강화 효과("+20%"), CostText 는 다음 레벨 비용 또는 "최대 레벨" 같은 안내.
	void Setup(EUT1WeaponStat InStat, int32 Level, int32 MaxLevel, const FText& BonusText, const FText& InCostText, bool bCanEnhance);

	UPROPERTY(BlueprintAssignable, Category = "Workbench")
	FUT1EnhanceRequested OnEnhanceRequested;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EnhanceButton;

private:
	void BuildDefaultLayout();

	UFUNCTION()
	void HandleClicked();

	EUT1WeaponStat Stat = EUT1WeaponStat::AttackPower;
};
