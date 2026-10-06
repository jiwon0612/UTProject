// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Combat/UT1DamageNumber.h"
#include "CJW/Combat/UT1DamageNumberWidget.h"
#include "Components/WidgetComponent.h"

AUT1DamageNumber::AUT1DamageNumber()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	Widget->SetupAttachment(Root);
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	Widget->SetWidgetClass(UUT1DamageNumberWidget::StaticClass());
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AUT1DamageNumber::Setup(float InDamage, bool bInCritical, const FLinearColor& InColor)
{
	Damage = InDamage;
	bCritical = bInCritical;
	Color = InColor;
}

void AUT1DamageNumber::BeginPlay()
{
	// 컴포넌트의 BeginPlay 에서 위젯 인스턴스가 만들어지므로 Super 다음에 채운다.
	Super::BeginPlay();

	SetLifeSpan(Lifetime);

	if (UUT1DamageNumberWidget* NumberWidget = Cast<UUT1DamageNumberWidget>(Widget->GetUserWidgetObject()))
	{
		// 소수점은 버리고 보여 준다. 치명타는 크게, 느낌표를 붙여 한눈에 구분되게.
		const int32 Rounded = FMath::Max(1, FMath::RoundToInt(Damage));
		const FText Text = bCritical
			? FText::Format(NSLOCTEXT("UT1Combat", "CritNumber", "{0}!"), Rounded)
			: FText::AsNumber(Rounded);

		NumberWidget->SetNumber(Text, Color, bCritical ? 34 : 22);
	}
}

void AUT1DamageNumber::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(Elapsed / Lifetime, 0.0f, 1.0f);

	// 위로 떠오르되 점점 느려진다 (1 - Alpha 를 속도에 곱함).
	AddActorWorldOffset(FVector(0.0f, 0.0f, RiseSpeed * (1.0f - Alpha) * DeltaSeconds));

	UUserWidget* UserWidget = Widget->GetUserWidgetObject();
	if (UserWidget == nullptr)
	{
		return;
	}

	// 앞 절반은 그대로 보이고 뒤 절반에서 빠르게 사라진다.
	UserWidget->SetRenderOpacity(1.0f - FMath::Square(Alpha));

	// 치명타는 처음 0.15초 동안 크게 튀어나왔다가 제 크기로 돌아온다.
	if (bCritical)
	{
		const float PopAlpha = FMath::Clamp(Elapsed / 0.15f, 0.0f, 1.0f);
		const float Scale = FMath::Lerp(1.8f, 1.0f, PopAlpha);
		UserWidget->SetRenderScale(FVector2D(Scale));
	}
}
