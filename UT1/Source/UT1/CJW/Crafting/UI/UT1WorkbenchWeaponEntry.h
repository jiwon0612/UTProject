// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1WorkbenchWeaponEntry.generated.h"

class UButton;
class UTextBlock;
class UUT1WeaponData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUT1WeaponEntrySelected, UUT1WeaponData*, Weapon);

/**
 * 작업대 왼쪽 목록의 무기 한 줄.
 *
 * 스스로 아무것도 판단하지 않는다. 표시할 값은 부모(UUT1WorkbenchWidget)가
 * 넣어 주고, 눌리면 어떤 무기가 눌렸는지만 알린다.
 *
 * 레이아웃: WBP 로 상속해 같은 이름의 위젯을 배치하면 그 레이아웃을 쓰고,
 * 비어 있으면 C++ 이 기본 레이아웃을 만든다 (BuildDefaultLayout).
 *   Button SelectButton
 *    └ HorizontalBox [ TextBlock NameText | TextBlock StateText ]
 */
UCLASS()
class UT1_API UUT1WorkbenchWeaponEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UUT1WeaponData* InWeapon, const FText& InStateText, bool bSelected);

	UPROPERTY(BlueprintAssignable, Category = "Workbench")
	FUT1WeaponEntrySelected OnSelected;

protected:
	virtual void NativeOnInitialized() override;

	// WBP 가 없거나 비어 있어도 동작하도록 전부 Optional 이다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StateText;

private:
	void BuildDefaultLayout();

	UFUNCTION()
	void HandleClicked();

	UPROPERTY()
	TObjectPtr<UUT1WeaponData> Weapon;
};
