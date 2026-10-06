// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1DamageNumberWidget.generated.h"

class UTextBlock;

/**
 * 데미지 숫자 하나. AUT1DamageNumber 의 WidgetComponent 안에 들어간다.
 *
 * 작업대 위젯과 같은 방식으로, WBP 레이아웃이 비어 있으면 C++ 이 기본 텍스트를 만든다.
 * 꾸미고 싶으면 이 클래스를 상속한 WBP 에 NumberText 라는 TextBlock 을 두면 된다.
 */
UCLASS()
class UT1_API UUT1DamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetNumber(const FText& Text, const FLinearColor& Color, int32 FontSize);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NumberText;
};
