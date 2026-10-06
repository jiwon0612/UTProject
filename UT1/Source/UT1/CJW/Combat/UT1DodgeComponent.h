// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UT1DodgeComponent.generated.h"

class UAnimMontage;
class AUT1Entity;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUT1DodgeEvent);

/**
 * 회피(대시) 한 번의 이동, 무적 시간, 쿨다운을 담당한다.
 *
 * 이동은 Root Motion Source 로 넣는다. 거리와 시간이 데이터로 고정되므로
 * 애니메이션과 무관하게 조절할 수 있고, 벽/경사 처리는 CharacterMovement 가 한다.
 * 몽타주는 보이는 모습만 맡는다.
 *
 * "무적이면 피해를 무시한다"는 규칙은 AUT1Entity 가 쥐고 있고, 이 컴포넌트는
 * 그 무적을 언제 켜고 끌지만 정한다. 그래서 회피하는 적에게도 그대로 붙일 수 있다.
 *
 * 공격 캔슬처럼 소유자마다 다른 사전 처리는 호출하는 쪽(플레이어/AI)의 몫이다.
 */
UCLASS(ClassGroup = (UT1), meta = (BlueprintSpawnableComponent))
class UT1_API UUT1DodgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUT1DodgeComponent();

	// Direction 이 0 이면 소유자가 바라보는 방향으로 회피한다.
	// 쿨다운 중이거나 이미 회피 중이면 아무것도 하지 않고 false.
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	bool TryDodge(FVector Direction);

	// 사망 등으로 회피를 즉시 끊는다. 무적도 함께 해제한다.
	void CancelDodge();

	UFUNCTION(BlueprintPure, Category = "Dodge")
	bool CanDodge() const;

	// 이동 구간과 후딜 구간을 모두 포함한다.
	UFUNCTION(BlueprintPure, Category = "Dodge")
	bool IsDodging() const { return bIsDodging; }

	// 쿨다운 UI 용. 사용 가능하면 0.
	UFUNCTION(BlueprintPure, Category = "Dodge")
	float GetCooldownRemaining() const;

	UPROPERTY(BlueprintAssignable, Category = "Dodge")
	FUT1DodgeEvent OnDodgeStarted;

	UPROPERTY(BlueprintAssignable, Category = "Dodge")
	FUT1DodgeEvent OnDodgeEnded;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 이동 거리(cm).
	UPROPERTY(EditAnywhere, Category = "Dodge", meta = (ClampMin = "0.0"))
	float Distance = 450.0f;

	// 실제로 미끄러지는 시간(초).
	UPROPERTY(EditAnywhere, Category = "Dodge", meta = (ClampMin = "0.05"))
	float Duration = 0.25f;

	// 이동이 끝난 뒤 제자리에서 자세를 회복하는 시간(초).
	// 대시는 빠르게 두면서 애니메이션이 지나치게 압축되지 않게 하려는 구간이다.
	// 회피 전체(Duration + RecoveryDuration) 동안 이동/공격 입력이 막힌다.
	UPROPERTY(EditAnywhere, Category = "Dodge", meta = (ClampMin = "0.0"))
	float RecoveryDuration = 0.2f;

	// 회피 시작부터 무적이 유지되는 시간(초). 0 이면 무적 없음.
	UPROPERTY(EditAnywhere, Category = "Dodge", meta = (ClampMin = "0.0"))
	float InvulnerableDuration = 0.3f;

	// 회피가 끝난 뒤 다시 쓸 수 있을 때까지의 시간(초).
	UPROPERTY(EditAnywhere, Category = "Dodge", meta = (ClampMin = "0.0"))
	float Cooldown = 0.5f;

	// 비어 있으면 재생을 건너뛴다. 이동은 몽타주가 없어도 동작한다.
	// 재생 속도는 Duration + RecoveryDuration 에 맞춰 자동으로 정해지므로 에셋의 Rate Scale 은 1 로 둔다.
	UPROPERTY(EditAnywhere, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeMontage;

private:
	AUT1Entity* GetOwnerEntity() const;

	void FinishDodge();
	void EndInvulnerable();

	bool bIsDodging = false;

	// 내가 켠 무적만 내가 끈다. 카운터 방식이라 짝이 어긋나면 영구 무적이 된다.
	bool bInvulnerableActive = false;

	double CooldownEndTime = 0.0;

	// 사망 시 진행 중인 이동을 지우려면 ID 가 필요하다.
	uint16 RootMotionSourceID = 0;

	FTimerHandle DodgeTimer;
	FTimerHandle InvulnerableTimer;
};
