// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UT1CombatFeedbackSubsystem.generated.h"

/**
 * 전투 연출을 한곳에 모으는 월드 서브시스템. 지금은 데미지 숫자를 띄운다.
 *
 * 엔티티는 "얼마나, 어떻게 맞았다"만 알리고, 그것을 어떻게 보여 줄지는 여기서 정한다.
 * 피격 규칙(AUT1Entity)과 연출이 섞이지 않게 하려는 분리다. 사운드나 카메라 흔들림을
 * 붙일 때도 엔티티를 고치지 않고 여기에 더하면 된다.
 *
 * WorldSubsystem 은 월드마다 엔진이 자동으로 하나씩 만든다. 레벨에 배치하거나
 * 참조를 연결할 필요 없이 GetWorld()->GetSubsystem<>() 으로 꺼내 쓴다.
 */
UCLASS()
class UT1_API UUT1CombatFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// bVictimIsPlayer: 플레이어가 맞으면 다른 색으로 보여 준다.
	void ShowDamageNumber(const FVector& Location, float Damage, bool bCritical, bool bVictimIsPlayer);

protected:
	// 에디터 프리뷰 월드 등에는 만들지 않는다. 실제 게임(PIE 포함)에서만 필요하다.
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};
