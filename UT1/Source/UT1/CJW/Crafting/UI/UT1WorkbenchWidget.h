// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CJW/Crafting/UT1CraftingTypes.h"
#include "UT1WorkbenchWidget.generated.h"

class UButton;
class UTextBlock;
class UPanelWidget;
class UUT1RunInventoryComponent;
class UUT1WeaponData;
class UUT1WorkbenchWeaponEntry;
class UUT1WorkbenchStatRow;

/**
 * 작업대 화면 (WBP_Workbench).
 *
 * 왼쪽: 설계도가 있는 무기 목록. 오른쪽: 고른 무기의 상세.
 *   - 미보유: 제작 비용 + [제작]
 *   - 보유:   강화 스탯 4줄 + [장착] [분해]
 *
 * 레이아웃은 두 가지 방식을 모두 지원한다.
 *   1) 이 C++ 클래스를 그대로 쓰면 BuildDefaultLayout 이 기본 화면을 조립한다.
 *   2) WBP 로 상속해 아래 이름의 위젯을 배치하면 BindWidgetOptional 이
 *      같은 이름끼리 연결하고, 기본 레이아웃은 만들지 않는다.
 * 그래서 지금은 에셋 없이 동작하고, 나중에 꾸밀 때 WBP 만 추가하면 된다.
 *
 * 규칙 판단은 전부 UUT1RunInventoryComponent 에 묻는다. 위젯이 "재료가
 * 충분한가"를 따로 계산하면 로직과 화면이 어긋날 수 있기 때문이다.
 */
UCLASS()
class UT1_API UUT1WorkbenchWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUT1WorkbenchWidget(const FObjectInitializer& ObjectInitializer);

	// 작업대가 부른다. 화면에 붙이고, 게임을 멈추고, 입력을 UI 로 돌린다.
	void Open(UUT1RunInventoryComponent* InInventory);

	// Open 에서 바꾼 것을 전부 되돌린다. 여는 쪽과 닫는 쪽을 한 클래스에 두어
	// 일시정지나 입력 모드가 풀리지 않은 채 남는 실수를 막는다.
	UFUNCTION(BlueprintCallable, Category = "Workbench")
	void Close();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// --- 왼쪽 ---
	// ScrollBox 나 VerticalBox 어느 쪽이든 된다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> WeaponList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MaterialsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	// --- 오른쪽 공통 ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedStateText;

	// 마지막 행동의 결과 ("재료가 부족합니다" 등).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;

	// --- 오른쪽: 미보유일 때 ---
	// 이 묶음 전체를 보였다 숨겼다 한다. 안에 CraftCostText 와 CraftButton 을 넣는다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CraftSection;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CraftCostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CraftButton;

	// --- 오른쪽: 보유 중일 때 ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> OwnedSection;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> StatList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EquipButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DismantleButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DismantleRefundText;

	// --- 목록 한 줄에 쓸 위젯 클래스. 기본값은 C++ 클래스, WBP 를 만들면 여기서 바꾼다 ---
	UPROPERTY(EditDefaultsOnly, Category = "Workbench")
	TSubclassOf<UUT1WorkbenchWeaponEntry> WeaponEntryClass;

	UPROPERTY(EditDefaultsOnly, Category = "Workbench")
	TSubclassOf<UUT1WorkbenchStatRow> StatRowClass;

private:
	void BuildDefaultLayout();

	// WBP 에서 필수 위젯을 빠뜨렸는지 검사한다. 빠졌으면 어떤 이름인지 로그로 알린다.
	bool ValidateLayout() const;

	// ValidateLayout 결과. false 면 모든 갱신과 열기를 건너뛰어 null 접근을 막는다.
	bool bLayoutValid = false;

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleWeaponSelected(UUT1WeaponData* Weapon);

	UFUNCTION()
	void HandleEnhanceRequested(EUT1WeaponStat Stat);

	UFUNCTION()
	void HandleCraftClicked();

	UFUNCTION()
	void HandleEquipClicked();

	UFUNCTION()
	void HandleDismantleClicked();

	UFUNCTION()
	void HandleCloseClicked();

	// 값이 바뀔 때마다 전체를 다시 그린다. 목록이 수십 개 이하라 충분하고,
	// 부분 갱신보다 "화면이 실제 상태와 다를" 여지가 없다.
	void Refresh();
	void RefreshMaterials();
	void RefreshWeaponList(const TArray<UUT1WeaponData*>& Weapons);
	void RefreshDetail();

	// 설계도가 있는 무기 + (설계도 없이 받은) 보유 무기.
	TArray<UUT1WeaponData*> GatherWeapons() const;

	FText GetWeaponStateText(const UUT1WeaponData* Weapon) const;

	// "고철 3/5, 전선 1/0" 처럼 필요량/보유량을 함께 보여 준다.
	FText FormatCosts(const TArray<FUT1MaterialCost>& Costs, bool bShowOwned) const;

	void ShowResult(EUT1WorkbenchResult Result);

	UPROPERTY()
	TObjectPtr<UUT1RunInventoryComponent> Inventory;

	UPROPERTY()
	TObjectPtr<UUT1WeaponData> SelectedWeapon;
};
