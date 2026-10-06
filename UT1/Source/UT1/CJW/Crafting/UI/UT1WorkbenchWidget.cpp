// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Crafting/UI/UT1WorkbenchWidget.h"
#include "CJW/Crafting/UI/UT1WorkbenchWeaponEntry.h"
#include "CJW/Crafting/UI/UT1WorkbenchStatRow.h"
#include "CJW/Crafting/UT1RunInventoryComponent.h"
#include "CJW/Crafting/UT1EnhancementRules.h"
#include "CJW/Crafting/UT1MaterialData.h"
#include "CJW/Weapons/UT1WeaponData.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "CJW/Crafting/UI/UT1WidgetBuildHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UT1.h"

#define LOCTEXT_NAMESPACE "UT1Workbench"

// ---------------------------------------------------------------- 레이아웃

UUT1WorkbenchWidget::UUT1WorkbenchWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// WBP 를 따로 만들지 않아도 목록이 그려지도록 C++ 클래스를 기본값으로 둔다.
	WeaponEntryClass = UUT1WorkbenchWeaponEntry::StaticClass();
	StatRowClass = UUT1WorkbenchStatRow::StaticClass();
}

void UUT1WorkbenchWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// WBP 레이아웃이 비어 있을 때만 기본 화면을 조립한다.
	if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultLayout();
	}

	bLayoutValid = ValidateLayout();
	if (bLayoutValid == false)
	{
		return;
	}

	// Esc / E 로 닫으려면 이 위젯이 키보드 포커스를 받을 수 있어야 한다.
	SetIsFocusable(true);

	CraftButton->OnClicked.AddDynamic(this, &UUT1WorkbenchWidget::HandleCraftClicked);
	EquipButton->OnClicked.AddDynamic(this, &UUT1WorkbenchWidget::HandleEquipClicked);
	DismantleButton->OnClicked.AddDynamic(this, &UUT1WorkbenchWidget::HandleDismantleClicked);
	CloseButton->OnClicked.AddDynamic(this, &UUT1WorkbenchWidget::HandleCloseClicked);
}

bool UUT1WorkbenchWidget::ValidateLayout() const
{
	struct FRequiredWidget
	{
		const UWidget* Widget;
		const TCHAR* Name;
	};

	const FRequiredWidget Required[] =
	{
		{ WeaponList.Get(),       TEXT("WeaponList") },
		{ MaterialsText.Get(),    TEXT("MaterialsText") },
		{ CloseButton.Get(),      TEXT("CloseButton") },
		{ SelectedNameText.Get(), TEXT("SelectedNameText") },
		{ CraftSection.Get(),     TEXT("CraftSection") },
		{ CraftCostText.Get(),    TEXT("CraftCostText") },
		{ CraftButton.Get(),      TEXT("CraftButton") },
		{ OwnedSection.Get(),     TEXT("OwnedSection") },
		{ StatList.Get(),         TEXT("StatList") },
		{ EquipButton.Get(),      TEXT("EquipButton") },
		{ DismantleButton.Get(),  TEXT("DismantleButton") },
	};

	bool bValid = true;
	for (const FRequiredWidget& Entry : Required)
	{
		if (Entry.Widget == nullptr)
		{
			UE_LOG(LogUT1, Error, TEXT("[Workbench] %s 에 '%s' 위젯이 없습니다. WBP 에서 같은 이름으로 배치하세요."),
				*GetClass()->GetName(), Entry.Name);
			bValid = false;
		}
	}
	return bValid;
}

void UUT1WorkbenchWidget::BuildDefaultLayout()
{
	using namespace UT1WidgetBuild;
	UWidgetTree* Tree = WidgetTree;

	auto AddToVBox = [](UVerticalBox* Box, UWidget* Child, const FMargin& ChildPadding = FMargin(0.0f, 0.0f, 0.0f, 8.0f))
	{
		UVerticalBoxSlot* ChildSlot = Box->AddChildToVerticalBox(Child);
		ChildSlot->SetPadding(ChildPadding);
		return ChildSlot;
	};

	auto MakeVBox = [Tree](FName Name)
	{
		return Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
	};

	auto MakeHBox = [Tree](FName Name)
	{
		return Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), Name);
	};

	// --- 바탕: 화면 전체를 어둡게 덮고 가운데에 창을 띄운다 ---
	UBorder* Dim = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
	Dim->SetHorizontalAlignment(HAlign_Center);
	Dim->SetVerticalAlignment(VAlign_Center);
	Tree->RootWidget = Dim;

	USizeBox* WindowSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WindowSize"));
	WindowSize->SetWidthOverride(1000.0f);
	WindowSize->SetHeightOverride(600.0f);
	Dim->SetContent(WindowSize);

	UBorder* Window = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Window"));
	Window->SetBrushColor(FLinearColor(0.04f, 0.04f, 0.05f, 0.97f));
	Window->SetPadding(FMargin(20.0f));
	WindowSize->SetContent(Window);

	UVerticalBox* Main = MakeVBox(TEXT("Main"));
	Window->SetContent(Main);

	// --- 머리: 제목 + 닫기 ---
	UHorizontalBox* Header = MakeHBox(TEXT("Header"));
	AddToVBox(Main, Header);

	UTextBlock* Title = MakeText(Tree, TEXT("Title"), LOCTEXT("Title", "작업대"), 24);
	UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(Title);
	TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleSlot->SetVerticalAlignment(VAlign_Center);

	CloseButton = MakeButton(Tree, TEXT("CloseButton"), LOCTEXT("Close", "닫기 (Esc)"));
	Header->AddChildToHorizontalBox(CloseButton)->SetVerticalAlignment(VAlign_Center);

	MaterialsText = MakeText(Tree, TEXT("MaterialsText"), FText::GetEmpty(), 14);
	MaterialsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f)));
	AddToVBox(Main, MaterialsText, FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	// --- 몸통: 왼쪽 목록 | 오른쪽 상세 ---
	UHorizontalBox* Body = MakeHBox(TEXT("Body"));
	AddToVBox(Main, Body, FMargin(0.0f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UScrollBox* List = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("WeaponList"));
	WeaponList = List;
	{
		UHorizontalBoxSlot* BodySlot = Body->AddChildToHorizontalBox(List);
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = 0.35f;
		BodySlot->SetSize(Size);
		BodySlot->SetPadding(FMargin(0.0f, 0.0f, 20.0f, 0.0f));
	}

	UVerticalBox* Detail = MakeVBox(TEXT("Detail"));
	{
		UHorizontalBoxSlot* BodySlot = Body->AddChildToHorizontalBox(Detail);
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = 0.65f;
		BodySlot->SetSize(Size);
	}

	SelectedNameText = MakeText(Tree, TEXT("SelectedNameText"), FText::GetEmpty(), 22);
	AddToVBox(Detail, SelectedNameText, FMargin(0.0f, 0.0f, 0.0f, 2.0f));

	SelectedStateText = MakeText(Tree, TEXT("SelectedStateText"), FText::GetEmpty(), 13);
	SelectedStateText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)));
	AddToVBox(Detail, SelectedStateText, FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	// --- 미보유: 제작 ---
	UVerticalBox* CraftBox = MakeVBox(TEXT("CraftSection"));
	CraftSection = CraftBox;
	AddToVBox(Detail, CraftBox);

	AddToVBox(CraftBox, MakeText(Tree, TEXT("CraftCostLabel"), LOCTEXT("CraftCostLabel", "필요 재료 (필요/보유)"), 13), FMargin(0.0f, 0.0f, 0.0f, 2.0f));
	CraftCostText = MakeText(Tree, TEXT("CraftCostText"), FText::GetEmpty(), 16);
	AddToVBox(CraftBox, CraftCostText, FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	CraftButton = MakeButton(Tree, TEXT("CraftButton"), LOCTEXT("Craft", "제작"), 16);
	AddToVBox(CraftBox, CraftButton)->SetHorizontalAlignment(HAlign_Left);

	// --- 보유: 강화 + 장착/분해 ---
	UVerticalBox* OwnedBox = MakeVBox(TEXT("OwnedSection"));
	OwnedSection = OwnedBox;
	AddToVBox(Detail, OwnedBox);

	AddToVBox(OwnedBox, MakeText(Tree, TEXT("EnhanceLabel"), LOCTEXT("EnhanceLabel", "강화 (비용: 필요/보유)"), 13), FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	UVerticalBox* Stats = MakeVBox(TEXT("StatList"));
	StatList = Stats;
	AddToVBox(OwnedBox, Stats, FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	UHorizontalBox* Actions = MakeHBox(TEXT("Actions"));
	AddToVBox(OwnedBox, Actions, FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	EquipButton = MakeButton(Tree, TEXT("EquipButton"), LOCTEXT("Equip", "장착"), 16);
	Actions->AddChildToHorizontalBox(EquipButton)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

	DismantleButton = MakeButton(Tree, TEXT("DismantleButton"), LOCTEXT("Dismantle", "분해"), 16);
	Actions->AddChildToHorizontalBox(DismantleButton);

	DismantleRefundText = MakeText(Tree, TEXT("DismantleRefundText"), FText::GetEmpty(), 12);
	DismantleRefundText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)));
	AddToVBox(OwnedBox, DismantleRefundText);

	// --- 결과 메시지 ---
	MessageText = MakeText(Tree, TEXT("MessageText"), FText::GetEmpty(), 15);
	AddToVBox(Detail, MessageText, FMargin(0.0f, 8.0f, 0.0f, 0.0f));
}

// ---------------------------------------------------------------- 열기 / 닫기

void UUT1WorkbenchWidget::Open(UUT1RunInventoryComponent* InInventory)
{
	if (bLayoutValid == false)
	{
		UE_LOG(LogUT1, Warning, TEXT("[Workbench] 레이아웃에 필수 위젯이 빠져 있어 작업대를 열지 않습니다. 위 에러 로그를 확인하세요."));
		return;
	}

	Inventory = InInventory;
	if (Inventory == nullptr)
	{
		UE_LOG(LogUT1, Warning, TEXT("[Workbench] 인벤토리가 없어 작업대를 열 수 없습니다."));
		return;
	}

	Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UUT1WorkbenchWidget::HandleInventoryChanged);

	// 처음에는 들고 있는 무기를 보여 준다. 강화하러 오는 경우가 가장 많다.
	SelectedWeapon = Inventory->GetEquippedWeapon();
	if (MessageText != nullptr)
	{
		MessageText->SetText(FText::GetEmpty());
	}

	AddToViewport(10);

	if (APlayerController* PC = GetOwningPlayer())
	{
		// UI 로 넘어가기 직전에 눌려 있던 키(이동키 등)가 눌린 채로 남지 않게 비운다.
		PC->FlushPressedKeys();

		// UI 전용 입력: 키와 클릭이 게임(이동/공격)으로 새지 않는다.
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}

	// 일시정지 중에도 UMG 는 입력과 그리기를 계속한다. 멈추는 것은 월드의 Tick 이다.
	UGameplayStatics::SetGamePaused(this, true);

	Refresh();
}

void UUT1WorkbenchWidget::Close()
{
	if (Inventory != nullptr)
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UUT1WorkbenchWidget::HandleInventoryChanged);
	}

	UGameplayStatics::SetGamePaused(this, false);

	if (APlayerController* PC = GetOwningPlayer())
	{
		// UI 에서 클릭하던 상태가 게임 입력에 "눌린 채"로 남으면 Enhanced Input 이
		// 다음 클릭의 Started 를 만들지 않아 공격이 나가지 않는다. 먼저 비운다.
		PC->FlushPressedKeys();

		// 게임 입력으로 되돌린다. FInputModeGameOnly 는 기본적으로 마우스 캡처를
		// 시작하는 첫 클릭을 삼키는데(bConsumeCaptureMouseDown), 이 프로젝트는
		// CapturePermanently_IncludingInitialMouseDown 설정이라 첫 클릭도 공격으로
		// 들어가야 한다. 프로젝트 설정과 같게 맞춘다.
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		PC->SetInputMode(InputMode);

		// 커서는 공격 방향에 쓰므로 계속 보인다.
		PC->SetShowMouseCursor(true);
	}

	RemoveFromParent();
}

void UUT1WorkbenchWidget::NativeDestruct()
{
	// 레벨 이동 등으로 Close 없이 사라지는 경우에도 구독은 끊는다.
	if (Inventory != nullptr)
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UUT1WorkbenchWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

FReply UUT1WorkbenchWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// UI 전용 입력 모드에서는 Enhanced Input 의 IA_Interact 가 오지 않는다.
	// 그래서 닫기 키는 위젯이 직접 받는다. E 는 여는 키와 맞춘 것.
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::E)
	{
		Close();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------- 갱신

void UUT1WorkbenchWidget::Refresh()
{
	if (Inventory == nullptr || bLayoutValid == false)
	{
		return;
	}

	const TArray<UUT1WeaponData*> Weapons = GatherWeapons();

	// 고른 무기가 목록에서 사라졌으면(분해 등) 첫 번째로 옮긴다.
	if (SelectedWeapon == nullptr || Weapons.Contains(SelectedWeapon) == false)
	{
		SelectedWeapon = Weapons.Num() > 0 ? Weapons[0] : nullptr;
	}

	RefreshMaterials();
	RefreshWeaponList(Weapons);
	RefreshDetail();
}

TArray<UUT1WeaponData*> UUT1WorkbenchWidget::GatherWeapons() const
{
	TArray<UUT1WeaponData*> Weapons;
	for (UUT1WeaponData* Weapon : Inventory->GetUnlockedBlueprints())
	{
		Weapons.AddUnique(Weapon);
	}
	// 보상으로 받은 무기처럼 설계도 없이 가진 무기도 강화/장착은 할 수 있어야 한다.
	for (const FUT1OwnedWeapon& Owned : Inventory->GetOwnedWeapons())
	{
		Weapons.AddUnique(Owned.WeaponData);
	}
	Weapons.Remove(nullptr);
	return Weapons;
}

void UUT1WorkbenchWidget::RefreshMaterials()
{
	TArray<FText> Parts;
	for (const TPair<TObjectPtr<UUT1MaterialData>, int32>& Pair : Inventory->GetMaterials())
	{
		if (Pair.Key != nullptr)
		{
			Parts.Add(FText::Format(LOCTEXT("MaterialCount", "{0} {1}"), Pair.Key->GetDisplayText(), Pair.Value));
		}
	}

	MaterialsText->SetText(Parts.Num() > 0
		? FText::Join(FText::FromString(TEXT("  ·  ")), Parts)
		: LOCTEXT("NoMaterials", "재료 없음"));
}

void UUT1WorkbenchWidget::RefreshWeaponList(const TArray<UUT1WeaponData*>& Weapons)
{
	WeaponList->ClearChildren();

	if (WeaponEntryClass == nullptr)
	{
		UE_LOG(LogUT1, Warning, TEXT("[Workbench] WeaponEntryClass 가 비어 있습니다. WBP_Workbench 의 Class Defaults 를 확인하세요."));
		return;
	}

	for (UUT1WeaponData* Weapon : Weapons)
	{
		UUT1WorkbenchWeaponEntry* Entry = CreateWidget<UUT1WorkbenchWeaponEntry>(this, WeaponEntryClass);
		Entry->Setup(Weapon, GetWeaponStateText(Weapon), Weapon == SelectedWeapon);
		Entry->OnSelected.AddDynamic(this, &UUT1WorkbenchWidget::HandleWeaponSelected);
		WeaponList->AddChild(Entry);
	}
}

void UUT1WorkbenchWidget::RefreshDetail()
{
	if (SelectedWeapon == nullptr)
	{
		SelectedNameText->SetText(LOCTEXT("NoWeapon", "설계도가 없습니다"));
		if (SelectedStateText != nullptr)
		{
			SelectedStateText->SetText(FText::GetEmpty());
		}
		CraftSection->SetVisibility(ESlateVisibility::Collapsed);
		OwnedSection->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SelectedNameText->SetText(SelectedWeapon->GetDisplayText());
	if (SelectedStateText != nullptr)
	{
		SelectedStateText->SetText(GetWeaponStateText(SelectedWeapon));
	}

	const bool bOwned = Inventory->IsWeaponOwned(SelectedWeapon);

	// 한 칸이 상태에 따라 바뀐다: 미보유면 제작, 보유면 강화/장착/분해.
	CraftSection->SetVisibility(bOwned ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	OwnedSection->SetVisibility(bOwned ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bOwned == false)
	{
		CraftCostText->SetText(SelectedWeapon->CraftCost.Num() > 0
			? FormatCosts(SelectedWeapon->CraftCost, true)
			: LOCTEXT("FreeCraft", "재료 필요 없음"));
		CraftButton->SetIsEnabled(Inventory->CanCraft(SelectedWeapon) == EUT1WorkbenchResult::Success);
		return;
	}

	// --- 강화 스탯 ---
	StatList->ClearChildren();
	const UUT1EnhancementRules* Rules = Inventory->GetEnhancementRules();

	if (StatRowClass != nullptr)
	{
		for (const EUT1WeaponStat Stat : UT1Crafting::AllWeaponStats)
		{
			const FUT1StatEnhanceRule* Rule = Rules != nullptr ? Rules->FindRule(Stat) : nullptr;
			const int32 Level = Inventory->GetStatLevel(SelectedWeapon, Stat);
			const int32 MaxLevel = Rule != nullptr ? Rule->MaxLevel : 0;
			const EUT1WorkbenchResult CanResult = Inventory->CanEnhance(SelectedWeapon, Stat);

			// 비용 칸에는 비용을 보여 주되, 강화가 불가능한 "구조적" 이유는 그 문구로 대신한다.
			FText CostText;
			if (CanResult == EUT1WorkbenchResult::MaxLevel || CanResult == EUT1WorkbenchResult::NoEnhanceRule)
			{
				CostText = UT1Crafting::GetResultMessage(CanResult);
			}
			else
			{
				TArray<FUT1MaterialCost> Costs;
				Inventory->GetEnhanceCost(SelectedWeapon, Stat, Costs);
				CostText = FormatCosts(Costs, true);
			}

			UUT1WorkbenchStatRow* Row = CreateWidget<UUT1WorkbenchStatRow>(this, StatRowClass);
			// 현재 효과. 치명타는 확률에 더하는 값이라 %p 로 구분한다.
			const int32 BonusPercent = FMath::RoundToInt(Inventory->GetStatBonus(SelectedWeapon, Stat) * 100.0f);
			const FText BonusText = FText::Format(Stat == EUT1WeaponStat::CritChance
				? LOCTEXT("BonusPercentPoint", "(+{0}%p)")
				: LOCTEXT("BonusPercent", "(+{0}%)"), BonusPercent);

			Row->Setup(Stat, Level, MaxLevel, BonusText, CostText, CanResult == EUT1WorkbenchResult::Success);
			Row->OnEnhanceRequested.AddDynamic(this, &UUT1WorkbenchWidget::HandleEnhanceRequested);
			StatList->AddChild(Row);
		}
	}
	else
	{
		UE_LOG(LogUT1, Warning, TEXT("[Workbench] StatRowClass 가 비어 있습니다. WBP_Workbench 의 Class Defaults 를 확인하세요."));
	}

	// --- 장착 / 분해 ---
	EquipButton->SetIsEnabled(Inventory->GetEquippedWeapon() != SelectedWeapon);
	DismantleButton->SetIsEnabled(Inventory->CanDismantle(SelectedWeapon) == EUT1WorkbenchResult::Success);

	if (DismantleRefundText != nullptr)
	{
		TArray<FUT1MaterialCost> Refund;
		Inventory->GetDismantleRefund(SelectedWeapon, Refund);
		DismantleRefundText->SetText(Refund.Num() > 0
			? FText::Format(LOCTEXT("RefundFormat", "분해 시 환급: {0}"), FormatCosts(Refund, false))
			: LOCTEXT("NoRefund", "분해 시 환급 없음"));
	}
}

FText UUT1WorkbenchWidget::GetWeaponStateText(const UUT1WeaponData* Weapon) const
{
	if (Inventory->GetEquippedWeapon() == Weapon)
	{
		return LOCTEXT("State_Equipped", "장착 중");
	}
	if (Inventory->IsWeaponOwned(Weapon))
	{
		return LOCTEXT("State_Owned", "보유");
	}
	return Inventory->CanCraft(Weapon) == EUT1WorkbenchResult::Success
		? LOCTEXT("State_Craftable", "제작 가능")
		: LOCTEXT("State_NotEnough", "재료 부족");
}

FText UUT1WorkbenchWidget::FormatCosts(const TArray<FUT1MaterialCost>& Costs, bool bShowOwned) const
{
	TArray<FText> Parts;
	for (const FUT1MaterialCost& Cost : Costs)
	{
		if (Cost.Material == nullptr)
		{
			continue;
		}

		if (bShowOwned)
		{
			Parts.Add(FText::Format(LOCTEXT("CostWithOwned", "{0} {1}/{2}"),
				Cost.Material->GetDisplayText(), Cost.Count, Inventory->GetMaterialCount(Cost.Material)));
		}
		else
		{
			Parts.Add(FText::Format(LOCTEXT("Cost", "{0} {1}"), Cost.Material->GetDisplayText(), Cost.Count));
		}
	}
	return FText::Join(FText::FromString(TEXT(", ")), Parts);
}

void UUT1WorkbenchWidget::ShowResult(EUT1WorkbenchResult Result)
{
	if (MessageText == nullptr)
	{
		return;
	}

	MessageText->SetText(UT1Crafting::GetResultMessage(Result));
	MessageText->SetColorAndOpacity(Result == EUT1WorkbenchResult::Success
		? FSlateColor(FLinearColor(0.3f, 1.0f, 0.4f))
		: FSlateColor(FLinearColor(1.0f, 0.35f, 0.3f)));
}

// ---------------------------------------------------------------- 이벤트

void UUT1WorkbenchWidget::HandleInventoryChanged()
{
	Refresh();
}

void UUT1WorkbenchWidget::HandleWeaponSelected(UUT1WeaponData* Weapon)
{
	SelectedWeapon = Weapon;
	if (MessageText != nullptr)
	{
		MessageText->SetText(FText::GetEmpty());
	}
	Refresh();
}

void UUT1WorkbenchWidget::HandleEnhanceRequested(EUT1WeaponStat Stat)
{
	// 성공하면 인벤토리가 OnInventoryChanged 를 보내서 화면이 다시 그려진다.
	ShowResult(Inventory->Enhance(SelectedWeapon, Stat));
}

void UUT1WorkbenchWidget::HandleCraftClicked()
{
	ShowResult(Inventory->Craft(SelectedWeapon));
}

void UUT1WorkbenchWidget::HandleEquipClicked()
{
	ShowResult(Inventory->EquipWeapon(SelectedWeapon));
}

void UUT1WorkbenchWidget::HandleDismantleClicked()
{
	ShowResult(Inventory->Dismantle(SelectedWeapon));
}

void UUT1WorkbenchWidget::HandleCloseClicked()
{
	Close();
}

#undef LOCTEXT_NAMESPACE
