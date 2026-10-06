// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1PlayerHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class AUT1Player;
class UUT1RunInventoryComponent;
class UUT1WeaponData;

/**
 * 전투 중 항상 떠 있는 플레이어 HUD (WBP_PlayerHUD).
 *
 *   왼쪽 위:   보유 재료 목록
 *   왼쪽 아래: 장착 무기 이름(+총 강화 단계)과 체력바
 *   가운데 아래: 상호작용 안내 ("E  작업대 사용")
 *
 * 값은 매 프레임 읽지 않고 델리게이트로 "바뀐 순간"에만 갱신한다.
 *   - 체력:      AUT1Entity::OnHealthChanged
 *   - 재료/강화: UUT1RunInventoryComponent::OnInventoryChanged
 *   - 장착:      UUT1RunInventoryComponent::OnEquippedWeaponChanged
 *   - 상호작용:  AUT1Player::OnFocusedInteractableChanged
 *
 * 레이아웃은 작업대 위젯과 같은 규칙이다. WBP 가 비어 있으면 C++ 이 기본 화면을
 * 조립하고, WBP 에 아래 이름의 위젯을 배치하면 그것을 쓴다. 전부 Optional 이라
 * 예를 들어 재료 칸을 빼고 체력바만 둔 WBP 도 동작한다.
 *
 * 위젯은 플레이어를 소유하지 않는다. AUT1HUD 가 "지금 조종 중인 플레이어"를
 * BindToPlayer 로 알려 주고, 리스폰으로 Pawn 이 바뀌면 다시 불러 준다.
 */
UCLASS()
class UT1_API UUT1PlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이전 대상의 구독을 끊고 새 대상에 구독한다. nullptr 이면 구독만 끊는다.
	void BindToPlayer(AUT1Player* NewPlayer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	// "73 / 100"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	// "기본검 +3". 맨손이면 "맨손".
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponText;

	// 재료 한 줄에 하나. 하나도 없으면 "재료 없음".
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MaterialsText;

	// "E  작업대 사용". 상호작용할 대상이 없으면 숨긴다.
	// 배경까지 함께 숨기고 싶으면 WBP 에서 InteractPromptPanel 로 감싼다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InteractPromptText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> InteractPromptPanel;

	// 안내 앞에 붙는 키 이름. 키 설정을 바꾸면 여기도 맞춘다.
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FText InteractKeyLabel = NSLOCTEXT("UT1PlayerHUD", "InteractKey", "E");

private:
	void BuildDefaultLayout();

	void Unbind();

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleEquippedWeaponChanged(UUT1WeaponData* NewWeapon);

	UFUNCTION()
	void HandleFocusedInteractableChanged(AActor* NewTarget);

	void RefreshWeapon();
	void RefreshMaterials();

	// 구독을 끊으려면 상대를 기억해야 한다. 상대가 먼저 파괴돼도
	// 댕글링 포인터가 되지 않게 약한 참조로 둔다.
	TWeakObjectPtr<AUT1Player> BoundPlayer;
	TWeakObjectPtr<UUT1RunInventoryComponent> BoundInventory;
};
