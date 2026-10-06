// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/UI/UT1DeathScreenWidget.h"
#include "CJW/Crafting/UI/UT1WidgetBuildHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "UT1DeathScreen"

void UUT1DeathScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// WBP 레이아웃이 비어 있을 때만 기본 화면을 조립한다.
	if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultLayout();
	}

	if (RestartButton != nullptr)
	{
		RestartButton->OnClicked.AddDynamic(this, &UUT1DeathScreenWidget::HandleRestartClicked);
	}
}

void UUT1DeathScreenWidget::BuildDefaultLayout()
{
	using namespace UT1WidgetBuild;
	UWidgetTree* Tree = WidgetTree;

	// 화면 전체를 어둡게 덮고 가운데에 제목과 버튼을 세로로 놓는다.
	UBorder* Dim = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
	Dim->SetHorizontalAlignment(HAlign_Center);
	Dim->SetVerticalAlignment(VAlign_Center);
	Tree->RootWidget = Dim;

	UVerticalBox* Content = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
	Dim->SetContent(Content);

	TitleText = MakeText(Tree, TEXT("TitleText"), LOCTEXT("Title", "사망"), 56);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.1f, 0.1f)));
	{
		UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(TitleText);
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 32.0f));
	}

	RestartButton = MakeButton(Tree, TEXT("RestartButton"), LOCTEXT("Restart", "다시 시작"), 20);
	Content->AddChildToVerticalBox(RestartButton)->SetHorizontalAlignment(HAlign_Center);
}

void UUT1DeathScreenWidget::Show()
{
	if (IsInViewport())
	{
		return;
	}

	// 작업대(10) 위에 덮이도록 더 높게 둔다.
	AddToViewport(20);

	if (APlayerController* PC = GetOwningPlayer())
	{
		// 죽기 직전에 눌려 있던 공격/이동 키가 남지 않게 비운다.
		PC->FlushPressedKeys();

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}
}

void UUT1DeathScreenWidget::Hide()
{
	if (IsInViewport() == false)
	{
		return;
	}

	RemoveFromParent();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->FlushPressedKeys();

		// 작업대 Close 와 같은 설정으로 게임 입력에 되돌린다.
		// 첫 클릭도 공격으로 들어가야 하므로 캡처 클릭을 삼키지 않는다.
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		PC->SetInputMode(InputMode);

		// 커서는 공격 방향에 쓰므로 계속 보인다.
		PC->SetShowMouseCursor(true);
	}
}

void UUT1DeathScreenWidget::HandleRestartClicked()
{
	OnRestartRequested.Broadcast();
}

#undef LOCTEXT_NAMESPACE
