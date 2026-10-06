// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/UI/UT1HUD.h"
#include "CJW/UI/UT1PlayerHUDWidget.h"
#include "CJW/UI/UT1DeathScreenWidget.h"
#include "CJW/Player/UT1Player.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UT1.h"

AUT1HUD::AUT1HUD()
{
	// WBP 를 따로 만들지 않아도 화면이 뜨도록 C++ 클래스를 기본값으로 둔다.
	PlayerHUDClass = UUT1PlayerHUDWidget::StaticClass();
	DeathScreenClass = UUT1DeathScreenWidget::StaticClass();
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
	}
	else
	{
		PlayerHUD = CreateWidget<UUT1PlayerHUDWidget>(PC, PlayerHUDClass);
		if (PlayerHUD != nullptr)
		{
			// 작업대(10)보다 아래에 깔리도록 0 으로 둔다.
			PlayerHUD->AddToViewport(0);
		}
	}

	PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &AUT1HUD::HandlePossessedPawnChanged);

	// HUD 보다 Pawn 빙의가 먼저 끝났을 수 있으므로 지금 대상으로 한 번 맞춘다.
	HandlePossessedPawnChanged(nullptr, PC->GetPawn());
}

void AUT1HUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DeathScreenTimer);

	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &AUT1HUD::HandlePossessedPawnChanged);
	}

	if (AUT1Player* Player = BoundPlayer.Get())
	{
		Player->OnDied.RemoveDynamic(this, &AUT1HUD::HandlePlayerDied);
	}

	if (PlayerHUD != nullptr)
	{
		PlayerHUD->RemoveFromParent();
		PlayerHUD = nullptr;
	}

	if (DeathScreen != nullptr)
	{
		DeathScreen->RemoveFromParent();
		DeathScreen = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AUT1HUD::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	AUT1Player* NewPlayer = Cast<AUT1Player>(NewPawn);

	// AUT1Player 가 아닌 Pawn(관전 카메라 등)이면 구독만 끊고 마지막 값을 남겨 둔다.
	if (PlayerHUD != nullptr)
	{
		PlayerHUD->BindToPlayer(NewPlayer);
	}

	if (BoundPlayer.Get() == NewPlayer)
	{
		return;
	}

	if (AUT1Player* OldPlayer = BoundPlayer.Get())
	{
		OldPlayer->OnDied.RemoveDynamic(this, &AUT1HUD::HandlePlayerDied);
	}
	BoundPlayer = NewPlayer;

	// 새 Pawn 으로 갈아탔다면 이전 죽음의 화면과 대기 타이머는 더 이상 의미가 없다.
	GetWorldTimerManager().ClearTimer(DeathScreenTimer);
	if (DeathScreen != nullptr)
	{
		DeathScreen->Hide();
	}

	if (NewPlayer != nullptr)
	{
		NewPlayer->OnDied.AddUniqueDynamic(this, &AUT1HUD::HandlePlayerDied);
	}
}

void AUT1HUD::HandlePlayerDied(AUT1Entity* Entity)
{
	// 쓰러지는 모습을 먼저 보여 주고 화면을 덮는다.
	if (DeathScreenDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(DeathScreenTimer, this, &AUT1HUD::ShowDeathScreen, DeathScreenDelay, false);
	}
	else
	{
		ShowDeathScreen();
	}
}

void AUT1HUD::ShowDeathScreen()
{
	if (DeathScreen == nullptr)
	{
		if (DeathScreenClass == nullptr)
		{
			UE_LOG(LogUT1, Warning, TEXT("[HUD] %s 의 DeathScreenClass 가 비어 있습니다."), *GetName());
			return;
		}

		DeathScreen = CreateWidget<UUT1DeathScreenWidget>(GetOwningPlayerController(), DeathScreenClass);
		if (DeathScreen == nullptr)
		{
			return;
		}
		DeathScreen->OnRestartRequested.AddUniqueDynamic(this, &AUT1HUD::RestartRun);
	}

	DeathScreen->Show();
}

void AUT1HUD::RestartRun()
{
	// 재시작 방식은 아직 정하지 않았다. 버튼 연결이 살아 있는지만 로그로 확인한다.
	UE_LOG(LogUT1, Log, TEXT("[HUD] 다시 시작 요청 - 아직 구현되지 않았습니다."));
}
