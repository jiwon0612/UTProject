// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CJW/Interaction/UT1Interactable.h"
#include "UT1Workbench.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UUT1WorkbenchWidget;

/**
 * 제작/강화/장착/분해를 하는 작업대. 특정 방에 배치하고 횟수 제한 없이 쓴다.
 *
 * 작업대는 "UI 를 여는 입구" 역할만 한다. 규칙은 플레이어의
 * UUT1RunInventoryComponent 가, 화면은 UUT1WorkbenchWidget 이 맡는다.
 * 그래서 나중에 상인 NPC 처럼 다른 입구가 생겨도 같은 위젯을 재사용할 수 있다.
 */
UCLASS()
class UT1_API AUT1Workbench : public AActor, public IUT1Interactable
{
	GENERATED_BODY()

public:
	AUT1Workbench();

	virtual void Interact_Implementation(AUT1Player* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Workbench")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 이 구 안에 플레이어가 들어오면 상호작용할 수 있다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Workbench")
	TObjectPtr<USphereComponent> InteractRange;

	// HUD 안내에 들어갈 문구. 범위 안에 들어오면 플레이어 HUD 가 "E  {문구}" 로 띄운다.
	UPROPERTY(EditAnywhere, Category = "Workbench")
	FText InteractionPrompt;

	// 기본값은 C++ 위젯(기본 레이아웃). 꾸민 WBP 를 만들면 여기서 바꾼다.
	UPROPERTY(EditAnywhere, Category = "Workbench")
	TSubclassOf<UUT1WorkbenchWidget> WidgetClass;
};
