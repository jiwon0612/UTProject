// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UT1Entity.generated.h"

class UAnimMontage;
class AUT1Entity;

// 체력바 UI 가 매 프레임 폴링하지 않도록, 바뀐 시점에만 알린다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUT1HealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUT1Died, AUT1Entity*, Entity);

/**
 * 플레이어와 적이 공유하는 베이스.
 *
 * 피격 규칙(체력 차감, 사망 판정, 중복 사망 방지)은 여기서 한 번만 정의하고,
 * 반응이 갈리는 부분만 HandleDamaged / HandleDeath 로 내려보낸다. 플레이어는
 * 죽을 때 입력을 끊어야 하고 적은 폐품을 떨궈야 하는데, 그 차이가 공통 규칙을
 * 오염시키지 않도록 분리한 것이다.
 *
 * 피해는 엔진 표준 TakeDamage 로 들어온다. 무기 트레이스가 ApplyPointDamage 를
 * 쓰고 있어서 중간 연결 코드가 따로 필요 없다.
 */
UCLASS()
class UT1_API AUT1Entity : public ACharacter
{
	GENERATED_BODY()

public:
	AUT1Entity();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	// 무적 중에는 TakeDamage 가 피해를 버린다.
	// bool 이 아니라 카운터인 이유: 회피 무적과 피격 후 무적처럼 원인이 겹칠 때
	// 먼저 끝난 쪽이 다른 쪽의 무적까지 풀어 버리지 않게 하려는 것이다.
	// Add 와 Remove 는 반드시 짝을 맞춘다.
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsInvulnerable() const { return InvulnerableCount > 0; }

	void AddInvulnerable() { ++InvulnerableCount; }
	void RemoveInvulnerable() { InvulnerableCount = FMath::Max(0, InvulnerableCount - 1); }

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FUT1HealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FUT1Died OnDied;

protected:
	// 개별 반응이 갈리는 자리. 공통 규칙은 TakeDamage 가 쥐고 있다.
	virtual void HandleDamaged(float ActualDamage, AActor* DamageCauser);
	virtual void HandleDeath(AActor* Killer);

	// 데미지 숫자 등 연출 요청. 연출 방식은 UUT1CombatFeedbackSubsystem 이 정한다.
	void ReportDamageFeedback(float ActualDamage, const struct FDamageEvent& DamageEvent);

	UPROPERTY(EditDefaultsOnly, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health")
	bool bIsDead = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health")
	int32 InvulnerableCount = 0;

	// 비어 있으면 재생을 건너뛴다.
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	TObjectPtr<UAnimMontage> DeathMontage;
};
