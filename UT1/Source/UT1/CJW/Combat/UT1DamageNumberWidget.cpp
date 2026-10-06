// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Combat/UT1DamageNumberWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

void UUT1DamageNumberWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
	{
		NumberText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NumberText"));
		WidgetTree->RootWidget = NumberText;

		// 배경이 밝든 어둡든 읽히도록 외곽선과 그림자를 같이 준다.
		FSlateFontInfo Font = NumberText->GetFont();
		Font.OutlineSettings.OutlineSize = 2;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f);
		NumberText->SetFont(Font);
		NumberText->SetShadowOffset(FVector2D(1.5f, 1.5f));
		NumberText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
		NumberText->SetJustification(ETextJustify::Center);
	}
}

void UUT1DamageNumberWidget::SetNumber(const FText& Text, const FLinearColor& Color, int32 FontSize)
{
	if (NumberText == nullptr)
	{
		return;
	}

	NumberText->SetText(Text);
	NumberText->SetColorAndOpacity(FSlateColor(Color));

	FSlateFontInfo Font = NumberText->GetFont();
	Font.Size = FontSize;
	NumberText->SetFont(Font);
}
