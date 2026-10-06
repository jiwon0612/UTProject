// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UT1HUD.generated.h"

class UUT1PlayerHUDWidget;
class UUT1DeathScreenWidget;
class AUT1Entity;
class AUT1Player;

/**
 * 화면 UI 의 진입점. 게임모드의 HUD Class 로 지정하면 엔진이 플레이어
 * 컨트롤러마다 하나씩 만들어 준다.
 *
 * HUD 위젯을 Pawn 이 아니라 여기서 만드는 이유: 로그라이크라 죽으면 Pawn 이
 * 새로 스폰된다. 위젯을 Pawn 이 들고 있으면 같이 사라졌다 다시 만들어지지만,
 * AHUD 는 컨트롤러와 수명이 같아서 위젯은 그대로 두고 대상만 바꿔 끼우면 된다.
 *
 * 이 클래스는 "누구를 보여 줄지, 어떤 화면을 언제 띄울지"만 정한다.
 * 무엇을 어떻게 그릴지는 위젯 몫이다.
 */
UCLASS()
class UT1_API AUT1HUD : public AHUD
{
	GENERATED_BODY()

public:
	AUT1HUD();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 기본값은 C++ 위젯(기본 레이아웃). 꾸민 WBP 를 만들면 BP_UT1HUD 에서 바꾼다.
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UUT1PlayerHUDWidget> PlayerHUDClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Death")
	TSubclassOf<UUT1DeathScreenWidget> DeathScreenClass;

	// 사망 후 화면이 뜨기까지 기다리는 시간(초). 쓰러지는 몽타주를 보여 주기 위한 여유.
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Death", meta = (ClampMin = "0.0"))
	float DeathScreenDelay = 2.0f;

	// [다시 시작] 버튼이 부른다. 현재 레벨을 다시 연다.
	// 런 흐름 담당(GameMode 나 런 매니저)이 생기면 그쪽 함수를 부르도록 옮긴다.
	UFUNCTION()
	void RestartRun();

private:
	// 빙의 대상이 바뀔 때(첫 스폰, 리스폰) 컨트롤러가 알려 준다.
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandlePlayerDied(AUT1Entity* Entity);

	void ShowDeathScreen();

	// 사망 구독 대상. 리스폰 때 이전 Pawn 의 구독을 끊으려고 기억한다.
	TWeakObjectPtr<AUT1Player> BoundPlayer;

	FTimerHandle DeathScreenTimer;

	UPROPERTY()
	TObjectPtr<UUT1PlayerHUDWidget> PlayerHUD;

	// 처음 죽을 때 만들고, 이후에는 재사용한다.
	UPROPERTY()
	TObjectPtr<UUT1DeathScreenWidget> DeathScreen;
};
