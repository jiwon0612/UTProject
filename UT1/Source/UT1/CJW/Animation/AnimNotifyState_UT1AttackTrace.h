// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_UT1AttackTrace.generated.h"

/**
 * 공격 몽타주에서 무기 판정이 살아 있는 구간을 표시한다.
 *
 * 순간 노티파이 두 개(Start / Finished)가 아니라 NotifyState 인 이유:
 * 콤보 전환이 이전 몽타주를 Montage_Play 로 덮어쓰기 때문에, 뒤쪽에 찍어 둔
 * 종료 노티파이는 재생이 거기까지 가지 못해 그냥 건너뛰어진다. 그러면 판정이
 * 켜진 채로 남는다. NotifyEnd 는 중단·블렌드아웃에도 호출되므로 그 구멍이 없다.
 *
 * 이 클래스는 신호만 전달한다. UAnimNotifyState 는 CDO 하나를 공유하므로
 * 인스턴스 상태(적중 목록, 직전 위치)를 여기에 담으면 무기 여러 개가 서로
 * 덮어쓴다. 실제 상태와 트레이스는 AUT1Weapon 이 들고 있다.
 */
UCLASS(const, hidecategories = Object, CollapseCategories, meta = (DisplayName = "UT1 Attack Trace"))
class UT1_API UAnimNotifyState_UT1AttackTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
