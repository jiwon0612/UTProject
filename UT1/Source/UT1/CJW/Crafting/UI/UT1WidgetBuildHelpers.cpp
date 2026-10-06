// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Crafting/UI/UT1WidgetBuildHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"

UTextBlock* UT1WidgetBuild::MakeText(UWidgetTree* Tree, FName Name, const FText& Text, int32 FontSize)
{
	UTextBlock* TextBlock = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	TextBlock->SetText(Text);

	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	return TextBlock;
}

UButton* UT1WidgetBuild::MakeButton(UWidgetTree* Tree, FName Name, const FText& Label, int32 FontSize)
{
	UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);

	// 버튼 기본 스타일은 밝은 배경이라 글자를 어둡게 둔다.
	UTextBlock* LabelText = MakeText(Tree, NAME_None, Label, FontSize);
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));

	if (UButtonSlot* Slot = Cast<UButtonSlot>(Button->AddChild(LabelText)))
	{
		Slot->SetPadding(FMargin(12.0f, 4.0f));
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetVerticalAlignment(VAlign_Center);
	}
	return Button;
}
