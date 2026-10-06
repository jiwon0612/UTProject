// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Crafting/UI/UT1WorkbenchStatRow.h"
#include "CJW/Crafting/UI/UT1WidgetBuildHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

void UUT1WorkbenchStatRow::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultLayout();
	}

	if (EnhanceButton != nullptr)
	{
		EnhanceButton->OnClicked.AddDynamic(this, &UUT1WorkbenchStatRow::HandleClicked);
	}
}

void UUT1WorkbenchStatRow::BuildDefaultLayout()
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	WidgetTree->RootWidget = Row;

	// 칸 너비 비율. 비용 문구가 가장 길다.
	auto AddCell = [Row](UWidget* Widget, float FillRatio)
	{
		UHorizontalBoxSlot* CellSlot = Row->AddChildToHorizontalBox(Widget);
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = FillRatio;
		CellSlot->SetSize(Size);
		CellSlot->SetVerticalAlignment(VAlign_Center);
		CellSlot->SetPadding(FMargin(0.0f, 3.0f, 8.0f, 3.0f));
	};

	StatNameText = UT1WidgetBuild::MakeText(WidgetTree, TEXT("StatNameText"), FText::GetEmpty(), 15);
	LevelText = UT1WidgetBuild::MakeText(WidgetTree, TEXT("LevelText"), FText::GetEmpty(), 15);
	CostText = UT1WidgetBuild::MakeText(WidgetTree, TEXT("CostText"), FText::GetEmpty(), 13);
	EnhanceButton = UT1WidgetBuild::MakeButton(WidgetTree, TEXT("EnhanceButton"),
		NSLOCTEXT("UT1Workbench", "EnhanceButton", "강화"));

	AddCell(StatNameText, 0.22f);
	AddCell(LevelText, 0.25f);
	AddCell(CostText, 0.35f);

	UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(EnhanceButton);
	ButtonSlot->SetVerticalAlignment(VAlign_Center);
}

void UUT1WorkbenchStatRow::Setup(EUT1WeaponStat InStat, int32 Level, int32 MaxLevel, const FText& BonusText, const FText& InCostText, bool bCanEnhance)
{
	Stat = InStat;

	if (StatNameText != nullptr)
	{
		StatNameText->SetText(UT1Crafting::GetStatDisplayName(Stat));
	}
	if (LevelText != nullptr)
	{
		LevelText->SetText(FText::Format(NSLOCTEXT("UT1Workbench", "StatLevel", "Lv {0}/{1}  {2}"), Level, MaxLevel, BonusText));
	}
	if (CostText != nullptr)
	{
		CostText->SetText(InCostText);
	}

	// 누를 수 없는 이유는 CostText 가 보여 준다 (재료 부족 / 최대 레벨).
	if (EnhanceButton != nullptr)
	{
		EnhanceButton->SetIsEnabled(bCanEnhance);
	}
}

void UUT1WorkbenchStatRow::HandleClicked()
{
	OnEnhanceRequested.Broadcast(Stat);
}
