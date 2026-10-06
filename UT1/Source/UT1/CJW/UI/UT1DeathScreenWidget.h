// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1DeathScreenWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUT1DeathScreenRestartRequested);

/**
 * 플레이어가 죽으면 뜨는 화면 (WBP_DeathScreen). 화면을 어둡게 덮고 "사망" 과
 * [다시 시작] 버튼만 보여 준다.
 *
 * 이 위젯은 "버튼이 눌렸다"는 사실만 알린다(OnRestartRequested). 실제로 런을
 * 어떻게 다시 시작할지는 게임 흐름의 몫이라 여기서 정하지 않는다.
 *
 * 레이아웃 규칙은 다른 UT1 위젯과 같다. WBP 가 비어 있으면 C++ 이 기본 화면을
 * 조립하고, WBP 에 TitleText / RestartButton 을 배치하면 그것을 쓴다.
 */
UCLASS()
class UT1_API UUT1DeathScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 화면에 붙이고 입력을 UI 로 돌린다.
	void Show();

	// Show 에서 바꾼 것을 되돌린다. 리스폰처럼 화면을 거둘 때 쓴다.
	void Hide();

	UPROPERTY(BlueprintAssignable, Category = "DeathScreen")
	FUT1DeathScreenRestartRequested OnRestartRequested;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RestartButton;

private:
	void BuildDefaultLayout();

	UFUNCTION()
	void HandleRestartClicked();
};
