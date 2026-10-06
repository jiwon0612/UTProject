// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/UI/UT1HUD.h"
#include "CJW/UI/UT1PlayerHUDWidget.h"
#include "CJW/Player/UT1Player.h"
#include "GameFramework/PlayerController.h"
#include "UT1.h"

AUT1HUD::AUT1HUD()
{
	// WBP 를 따로 만들지 않아도 HUD 가 뜨도록 C++ 클래스를 기본값으로 둔다.
	PlayerHUDClass = UUT1PlayerHUDWidget::StaticClass();
}

void AUT1HUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (PC == nullptr || PC->IsLocalController() == false)
	{
		return;
	}

	if (PlayerHUDClass == nullptr)
	{
		UE_LOG(LogUT1, Warning, TEXT("[HUD] %s 의 PlayerHUDClass 가 비어 있습니다."), *GetName());
		return;
	}

	PlayerHUD = CreateWidget<UUT1PlayerHUDWidget>(PC, PlayerHUDClass);
	if (PlayerHUD == nullptr)
	{
		return;
	}

	// 작업대(10)보다 아래에 깔리도록 0 으로 둔다.
	PlayerHUD->AddToViewport(0);

	PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &AUT1HUD::HandlePossessedPawnChanged);

	// HUD 보다 Pawn 빙의가 먼저 끝났을 수 있으므로 지금 대상으로 한 번 맞춘다.
	HandlePossessedPawnChanged(nullptr, PC->GetPawn());
}

void AUT1HUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &AUT1HUD::HandlePossessedPawnChanged);
	}

	if (PlayerHUD != nullptr)
	{
		PlayerHUD->RemoveFromParent();
		PlayerHUD = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AUT1HUD::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (PlayerHUD == nullptr)
	{
		return;
	}

	// AUT1Player 가 아닌 Pawn(관전 카메라 등)이면 구독만 끊고 마지막 값을 남겨 둔다.
	PlayerHUD->BindToPlayer(Cast<AUT1Player>(NewPawn));
}
