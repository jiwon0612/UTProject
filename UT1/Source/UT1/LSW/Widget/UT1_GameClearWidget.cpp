#include "LSW/Widget/UT1_GameClearWidget.h"
#include "CJW/Crafting/UI/UT1WidgetBuildHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "UT1GameClear"

void UUT1_GameClearWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultLayout();
	}
}

void UUT1_GameClearWidget::BuildDefaultLayout()
{
	using namespace UT1WidgetBuild;
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
	Dim->SetHorizontalAlignment(HAlign_Center);
	Dim->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Dim;

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
	Dim->SetContent(Content);
	UTextBlock* Title = MakeText(WidgetTree, TEXT("Title"), LOCTEXT("Title", "게임 클리어!"), 64);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.78f, 0.2f)));
	Content->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);
	UTextBlock* Subtitle = MakeText(WidgetTree, TEXT("Subtitle"), LOCTEXT("Subtitle", "최종 보스를 쓰러뜨렸어!"), 24);
	UVerticalBoxSlot* SubtitleSlot = Content->AddChildToVerticalBox(Subtitle);
	SubtitleSlot->SetHorizontalAlignment(HAlign_Center);
	SubtitleSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
}

void UUT1_GameClearWidget::Show()
{
	if (!IsInViewport())
	{
		AddToViewport(100);
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->FlushPressedKeys();
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}
}

#undef LOCTEXT_NAMESPACE
