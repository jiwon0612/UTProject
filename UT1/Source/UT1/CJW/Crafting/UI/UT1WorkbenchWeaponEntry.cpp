// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Crafting/UI/UT1WorkbenchWeaponEntry.h"
#include "CJW/Crafting/UI/UT1WidgetBuildHelpers.h"
#include "CJW/Weapons/UT1WeaponData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

void UUT1WorkbenchWeaponEntry::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// WBP 레이아웃이 없을 때만 기본 레이아웃을 만든다.
	// NativeOnInitialized 는 위젯 트리가 준비된 뒤, 화면용 Slate 위젯이 만들어지기 전에
	// 한 번만 불리므로 트리를 조립하기에 알맞은 시점이다.
	if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultLayout();
	}

	// NativeConstruct 가 아니라 여기서 바인딩한다. Construct 는 화면에 다시 붙을 때마다
	// 불려서 같은 함수가 여러 번 바인딩될 수 있지만, OnInitialized 는 한 번만 불린다.
	if (SelectButton != nullptr)
	{
		SelectButton->OnClicked.AddDynamic(this, &UUT1WorkbenchWeaponEntry::HandleClicked);
	}
}

void UUT1WorkbenchWeaponEntry::BuildDefaultLayout()
{
	SelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SelectButton"));
	WidgetTree->RootWidget = SelectButton;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(SelectButton->AddChild(Row)))
	{
		ButtonSlot->SetPadding(FMargin(8.0f, 6.0f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	NameText = UT1WidgetBuild::MakeText(WidgetTree, TEXT("NameText"), FText::GetEmpty(), 16);
	if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameText))
	{
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
	}

	StateText = UT1WidgetBuild::MakeText(WidgetTree, TEXT("StateText"), FText::GetEmpty(), 12);
	if (UHorizontalBoxSlot* StateSlot = Row->AddChildToHorizontalBox(StateText))
	{
		StateSlot->SetVerticalAlignment(VAlign_Center);
	}

	// 기본 버튼 배경이 밝아서 글자를 어둡게 둔다. 선택 표시는 Setup 에서 덮어쓴다.
	StateText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.2f, 0.2f)));
}

void UUT1WorkbenchWeaponEntry::Setup(UUT1WeaponData* InWeapon, const FText& InStateText, bool bSelected)
{
	Weapon = InWeapon;

	if (NameText != nullptr)
	{
		NameText->SetText(Weapon != nullptr ? Weapon->GetDisplayText() : FText::GetEmpty());

		// 선택된 줄은 글자색으로만 구분한다. 버튼 스타일은 WBP 에서 자유롭게 꾸밀 수 있게 건드리지 않는다.
		NameText->SetColorAndOpacity(bSelected
			? FSlateColor(FLinearColor(0.85f, 0.45f, 0.0f))
			: FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	}

	if (StateText != nullptr)
	{
		StateText->SetText(InStateText);
	}
}

void UUT1WorkbenchWeaponEntry::HandleClicked()
{
	OnSelected.Broadcast(Weapon);
}
