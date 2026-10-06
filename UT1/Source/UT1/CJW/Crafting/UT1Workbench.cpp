// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Crafting/UT1Workbench.h"
#include "CJW/Crafting/UI/UT1WorkbenchWidget.h"
#include "CJW/Player/UT1Player.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "UT1.h"

AUT1Workbench::AUT1Workbench()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	InteractRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractRange"));
	InteractRange->SetupAttachment(Mesh);
	InteractRange->SetSphereRadius(200.0f);
	// 막지는 않고 겹침만 일으킨다. 플레이어 캡슐(Pawn)과 겹치면
	// 양쪽 액터 모두 NotifyActorBeginOverlap 을 받는다.
	InteractRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	PromptText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PromptText"));
	PromptText->SetupAttachment(Mesh);
	PromptText->SetText(NSLOCTEXT("UT1Workbench", "Prompt", "E: 작업대"));
	PromptText->SetHorizontalAlignment(EHTA_Center);
	PromptText->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	// 텍스트 앞면(+X)이 카메라를 보게 한다. 플레이어 SpringArm 이 (-50, 45, 0) 이라
	// 카메라는 Yaw 225, 위로 50도 방향에 있다. 카메라 각도를 바꾸면 여기도 맞춘다.
	PromptText->SetRelativeRotation(FRotator(50.0f, 225.0f, 0.0f));
	PromptText->SetHiddenInGame(true);

	// 위젯은 C++ 기본 레이아웃으로 바로 동작한다. WBP 로 꾸미면 BP 에서 바꾼다.
	WidgetClass = UUT1WorkbenchWidget::StaticClass();
}

void AUT1Workbench::Interact_Implementation(AUT1Player* Interactor)
{
	if (Interactor == nullptr)
	{
		return;
	}

	if (WidgetClass == nullptr)
	{
		UE_LOG(LogUT1, Warning, TEXT("[Workbench] %s 의 WidgetClass 가 비어 있습니다."), *GetName());
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	if (PC == nullptr)
	{
		return;
	}

	// 열 때마다 새로 만든다. 닫으면 RemoveFromParent 로 떨어지고 GC 가 치운다.
	// 한 번 만들어 재사용할 수도 있지만, 매번 새로 그리는 지금 구조에서는 이득이 작다.
	UUT1WorkbenchWidget* Widget = CreateWidget<UUT1WorkbenchWidget>(PC, WidgetClass);
	if (Widget != nullptr)
	{
		Widget->Open(Interactor->GetRunInventory());
	}
}

void AUT1Workbench::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (Cast<AUT1Player>(OtherActor) != nullptr)
	{
		PromptText->SetHiddenInGame(false);
	}
}

void AUT1Workbench::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (Cast<AUT1Player>(OtherActor) != nullptr)
	{
		PromptText->SetHiddenInGame(true);
	}
}
