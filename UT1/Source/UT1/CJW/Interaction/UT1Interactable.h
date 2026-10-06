// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UT1Interactable.generated.h"

class AUT1Player;

UINTERFACE(MinimalAPI, Blueprintable)
class UUT1Interactable : public UInterface
{
	GENERATED_BODY()
};

/**
 * E 키로 상호작용할 수 있는 대상 (작업대, 이후 상자/NPC/포털 등).
 *
 * 플레이어는 범위 안에 들어온 액터가 이 인터페이스를 구현했는지만 보고
 * Interact 를 부른다. 상대가 작업대인지 상자인지 몰라도 되므로, 대상이
 * 늘어나도 플레이어 코드는 바뀌지 않는다.
 *
 * 범위 판정은 플레이어 쪽이 한다(캡슐과 대상의 겹침). 대상은 겹침이
 * 일어날 콜리전만 갖고 있으면 된다.
 *
 * BlueprintNativeEvent 라서 C++ 에서는 Interact_Implementation 을 구현하고,
 * 호출은 IUT1Interactable::Execute_Interact(대상, 플레이어) 로 한다.
 * 블루프린트 액터도 이 인터페이스를 붙여 구현할 수 있다.
 */
class UT1_API IUT1Interactable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AUT1Player* Interactor);
};
