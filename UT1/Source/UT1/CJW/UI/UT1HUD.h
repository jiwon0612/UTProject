// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UT1HUD.generated.h"

class UUT1PlayerHUDWidget;

/**
 * 화면 UI 의 진입점. 게임모드의 HUD Class 로 지정하면 엔진이 플레이어
 * 컨트롤러마다 하나씩 만들어 준다.
 *
 * HUD 위젯을 Pawn 이 아니라 여기서 만드는 이유: 로그라이크라 죽으면 Pawn 이
 * 새로 스폰된다. 위젯을 Pawn 이 들고 있으면 같이 사라졌다 다시 만들어지지만,
 * AHUD 는 컨트롤러와 수명이 같아서 위젯은 그대로 두고 대상만 바꿔 끼우면 된다.
 *
 * 이 클래스는 "누구를 보여 줄지"만 정한다. 무엇을 어떻게 그릴지는 위젯 몫이다.
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

private:
	// 빙의 대상이 바뀔 때(첫 스폰, 리스폰) 컨트롤러가 알려 준다.
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UPROPERTY()
	TObjectPtr<UUT1PlayerHUDWidget> PlayerHUD;
};
