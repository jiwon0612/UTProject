// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "UT1DamageType_Critical.generated.h"

/**
 * 치명타 피해 표시.
 *
 * DamageType 은 "어떤 종류의 피해인가"를 피해량과 함께 넘기는 언리얼 표준 통로다.
 * 무기가 치명타를 판정하면 ApplyPointDamage 에 이 클래스를 실어 보내고,
 * 맞는 쪽(AUT1Entity::TakeDamage)은 DamageEvent.DamageTypeClass 로 치명타인지 안다.
 * 덕분에 무기와 피격자 사이에 별도 함수나 인자를 추가하지 않아도 된다.
 *
 * 클래스 자체가 정보라 내용은 비어 있다. 나중에 속성 피해(불, 독)도 같은 방식으로 늘릴 수 있다.
 */
UCLASS()
class UT1_API UUT1DamageType_Critical : public UDamageType
{
	GENERATED_BODY()
};
