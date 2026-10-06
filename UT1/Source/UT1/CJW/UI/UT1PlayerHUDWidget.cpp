// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/UI/UT1PlayerHUDWidget.h"
#include "CJW/Player/UT1Player.h"
#include "CJW/Crafting/UT1RunInventoryComponent.h"
#include "CJW/Crafting/UT1MaterialData.h"
#include "CJW/Weapons/UT1WeaponData.h"
#include "CJW/Interaction/UT1Interactable.h"
#include "CJW/Crafting/UI/UT1WidgetBuildHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "UT1PlayerHUD"

// ---------------------------------------------------------------- 레이아웃

void UUT1PlayerHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// WBP 레이아웃이 비어 있을 때만 기본 화면을 조립한다.
	if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultLayout();
	}

	// 플레이어가 연결되기 전에는 빈 안내 상자가 떠 있지 않게 숨겨 둔다.
	HandleFocusedInteractableChanged(nullptr);
}

void UUT1PlayerHUDWidget::BuildDefaultLayout()
{
	using namespace UT1WidgetBuild;
	UWidgetTree* Tree = WidgetTree;

	// 화면 모서리에 붙여 놓을 것이라 앵커를 쓸 수 있는 CanvasPanel 을 루트로 둔다.
	UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	Tree->RootWidget = Root;

	// --- 왼쪽 위: 재료 ---
	UBorder* MaterialsPanel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MaterialsPanel"));
	MaterialsPanel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));
	MaterialsPanel->SetPadding(FMargin(12.0f, 8.0f));
	{
		UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(MaterialsPanel);
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		PanelSlot->SetPosition(FVector2D(24.0f, 24.0f));
		PanelSlot->SetAutoSize(true);
	}

	MaterialsText = MakeText(Tree, TEXT("MaterialsText"), FText::GetEmpty(), 14);
	MaterialsPanel->SetContent(MaterialsText);

	// --- 왼쪽 아래: 무기 이름 + 체력바 ---
	UVerticalBox* Status = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Status"));
	{
		// 앵커와 정렬을 모두 왼쪽 아래(0,1)로 두면, 위젯의 왼쪽 아래 모서리가
		// 화면 왼쪽 아래에 붙는다. 해상도가 바뀌어도 위치가 유지된다.
		UCanvasPanelSlot* StatusSlot = Root->AddChildToCanvas(Status);
		StatusSlot->SetAnchors(FAnchors(0.0f, 1.0f));
		StatusSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		StatusSlot->SetPosition(FVector2D(24.0f, -24.0f));
		StatusSlot->SetAutoSize(true);
	}

	WeaponText = MakeText(Tree, TEXT("WeaponText"), FText::GetEmpty(), 18);
	Status->AddChildToVerticalBox(WeaponText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	USizeBox* BarSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HealthBarSize"));
	BarSize->SetWidthOverride(320.0f);
	BarSize->SetHeightOverride(24.0f);
	Status->AddChildToVerticalBox(BarSize);

	// 바 위에 숫자를 겹쳐 그리려고 Overlay 를 쓴다.
	UOverlay* BarOverlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HealthBarOverlay"));
	BarSize->SetContent(BarOverlay);

	HealthBar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
	HealthBar->SetFillColorAndOpacity(FLinearColor(0.75f, 0.08f, 0.08f));
	{
		UOverlaySlot* BarSlot = BarOverlay->AddChildToOverlay(HealthBar);
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(VAlign_Fill);
	}

	HealthText = MakeText(Tree, TEXT("HealthText"), FText::GetEmpty(), 13);
	{
		UOverlaySlot* TextSlot = BarOverlay->AddChildToOverlay(HealthText);
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	// --- 가운데 아래: 상호작용 안내 ---
	UBorder* PromptPanel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InteractPromptPanel"));
	PromptPanel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
	PromptPanel->SetPadding(FMargin(16.0f, 8.0f));
	InteractPromptPanel = PromptPanel;
	{
		// 가로 가운데(0.5), 세로 아래(1) 기준. 체력바와 겹치지 않게 조금 띄운다.
		UCanvasPanelSlot* PromptSlot = Root->AddChildToCanvas(PromptPanel);
		PromptSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		PromptSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		PromptSlot->SetPosition(FVector2D(0.0f, -120.0f));
		PromptSlot->SetAutoSize(true);
	}

	InteractPromptText = MakeText(Tree, TEXT("InteractPromptText"), FText::GetEmpty(), 18);
	PromptPanel->SetContent(InteractPromptText);
}

// ---------------------------------------------------------------- 바인딩

void UUT1PlayerHUDWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

void UUT1PlayerHUDWidget::BindToPlayer(AUT1Player* NewPlayer)
{
	if (BoundPlayer.Get() == NewPlayer)
	{
		return;
	}

	Unbind();

	if (NewPlayer == nullptr)
	{
		// 조종 중인 플레이어가 없으면 누를 수 있는 대상도 없다.
		HandleFocusedInteractableChanged(nullptr);
		return;
	}

	BoundPlayer = NewPlayer;
	NewPlayer->OnHealthChanged.AddUniqueDynamic(this, &UUT1PlayerHUDWidget::HandleHealthChanged);
	NewPlayer->OnFocusedInteractableChanged.AddUniqueDynamic(this, &UUT1PlayerHUDWidget::HandleFocusedInteractableChanged);

	if (UUT1RunInventoryComponent* Inventory = NewPlayer->GetRunInventory())
	{
		BoundInventory = Inventory;
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UUT1PlayerHUDWidget::HandleInventoryChanged);
		Inventory->OnEquippedWeaponChanged.AddUniqueDynamic(this, &UUT1PlayerHUDWidget::HandleEquippedWeaponChanged);
	}

	// 델리게이트는 "다음 변화"부터만 알려 준다. 플레이어의 BeginPlay 브로드캐스트나
	// 시작 무기 장착은 이미 지나갔을 수 있으므로, 구독 직후 현재 값을 직접 읽어 맞춘다.
	HandleHealthChanged(NewPlayer->GetHealth(), NewPlayer->GetMaxHealth());
	RefreshWeapon();
	RefreshMaterials();
	HandleFocusedInteractableChanged(NewPlayer->GetFocusedInteractable());
}

void UUT1PlayerHUDWidget::Unbind()
{
	if (AUT1Player* Player = BoundPlayer.Get())
	{
		Player->OnHealthChanged.RemoveDynamic(this, &UUT1PlayerHUDWidget::HandleHealthChanged);
		Player->OnFocusedInteractableChanged.RemoveDynamic(this, &UUT1PlayerHUDWidget::HandleFocusedInteractableChanged);
	}

	if (UUT1RunInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UUT1PlayerHUDWidget::HandleInventoryChanged);
		Inventory->OnEquippedWeaponChanged.RemoveDynamic(this, &UUT1PlayerHUDWidget::HandleEquippedWeaponChanged);
	}

	BoundPlayer.Reset();
	BoundInventory.Reset();
}

// ---------------------------------------------------------------- 갱신

void UUT1PlayerHUDWidget::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	if (HealthBar != nullptr)
	{
		HealthBar->SetPercent(MaxHealth > 0.0f ? NewHealth / MaxHealth : 0.0f);
	}

	if (HealthText != nullptr)
	{
		// 0.4 처럼 소수점이 남은 체력이 "0" 으로 보이면 죽은 것처럼 읽히므로 올림한다.
		HealthText->SetText(FText::Format(LOCTEXT("HealthFormat", "{0} / {1}"),
			FText::AsNumber(FMath::CeilToInt(NewHealth)),
			FText::AsNumber(FMath::CeilToInt(MaxHealth))));
	}
}

void UUT1PlayerHUDWidget::HandleInventoryChanged()
{
	// 강화도 이 알림으로 들어오므로 무기 칸의 "+N" 도 함께 갱신한다.
	RefreshWeapon();
	RefreshMaterials();
}

void UUT1PlayerHUDWidget::HandleEquippedWeaponChanged(UUT1WeaponData* NewWeapon)
{
	RefreshWeapon();
}

void UUT1PlayerHUDWidget::HandleFocusedInteractableChanged(AActor* NewTarget)
{
	const bool bHasTarget = NewTarget != nullptr && NewTarget->Implements<UUT1Interactable>();

	// 패널이 있으면 패널째로, 없으면 글자만 숨긴다. Collapsed 는 자리도 차지하지 않는다.
	UWidget* ToggleTarget = InteractPromptPanel != nullptr ? InteractPromptPanel.Get() : InteractPromptText.Get();
	if (ToggleTarget != nullptr)
	{
		ToggleTarget->SetVisibility(bHasTarget ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (bHasTarget && InteractPromptText != nullptr)
	{
		// 문구는 대상에게 묻는다. HUD 는 대상이 작업대인지 상자인지 몰라도 된다.
		const FText Action = IUT1Interactable::Execute_GetInteractionPrompt(NewTarget);
		InteractPromptText->SetText(FText::Format(LOCTEXT("InteractPromptFormat", "{0}  {1}"), InteractKeyLabel, Action));
	}
}

void UUT1PlayerHUDWidget::RefreshWeapon()
{
	if (WeaponText == nullptr)
	{
		return;
	}

	const UUT1RunInventoryComponent* Inventory = BoundInventory.Get();
	const UUT1WeaponData* Weapon = Inventory != nullptr ? Inventory->GetEquippedWeapon() : nullptr;
	if (Weapon == nullptr)
	{
		WeaponText->SetText(LOCTEXT("Unarmed", "맨손"));
		return;
	}

	// 스탯별 레벨은 작업대에서 자세히 보이므로, HUD 에는 "얼마나 키웠나" 합계만 띄운다.
	int32 TotalLevel = 0;
	if (const FUT1OwnedWeapon* Owned = Inventory->GetEquippedOwnedWeapon())
	{
		for (const TPair<EUT1WeaponStat, int32>& Pair : Owned->StatLevels)
		{
			TotalLevel += Pair.Value;
		}
	}

	WeaponText->SetText(TotalLevel > 0
		? FText::Format(LOCTEXT("WeaponWithLevel", "{0} +{1}"), Weapon->GetDisplayText(), FText::AsNumber(TotalLevel))
		: Weapon->GetDisplayText());
}

void UUT1PlayerHUDWidget::RefreshMaterials()
{
	if (MaterialsText == nullptr)
	{
		return;
	}

	const UUT1RunInventoryComponent* Inventory = BoundInventory.Get();
	if (Inventory == nullptr || Inventory->GetMaterials().Num() == 0)
	{
		MaterialsText->SetText(LOCTEXT("NoMaterials", "재료 없음"));
		return;
	}

	// TMap 순회 순서는 보장되지 않아서, 재료를 얻을 때마다 줄 순서가 바뀌어 보일 수 있다.
	// 이름순으로 고정해 같은 재료가 늘 같은 줄에 있게 한다.
	TArray<TPair<FText, int32>> Rows;
	for (const TPair<TObjectPtr<UUT1MaterialData>, int32>& Pair : Inventory->GetMaterials())
	{
		if (Pair.Key != nullptr && Pair.Value > 0)
		{
			Rows.Emplace(Pair.Key->GetDisplayText(), Pair.Value);
		}
	}
	Rows.Sort([](const TPair<FText, int32>& A, const TPair<FText, int32>& B)
	{
		return A.Key.CompareTo(B.Key) < 0;
	});

	TArray<FText> Lines;
	for (const TPair<FText, int32>& Row : Rows)
	{
		Lines.Add(FText::Format(LOCTEXT("MaterialRow", "{0}  x{1}"), Row.Key, FText::AsNumber(Row.Value)));
	}
	MaterialsText->SetText(FText::Join(FText::FromString(TEXT("\n")), Lines));
}

#undef LOCTEXT_NAMESPACE
