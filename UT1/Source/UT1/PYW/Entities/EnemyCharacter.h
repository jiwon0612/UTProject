#pragma once

#include "CoreMinimal.h"
#include "CJW/Entities/UT1Entity.h"
#include "EnemyCharacter.generated.h"

UENUM(BlueprintType)
enum class EEnemyAIState : uint8
{
	Idle,
	Walk,
	Chase,
	Attack
};

UENUM(BlueprintType)
enum class EEnemyCombatType : uint8
{
	Melee,
	Ranged
};

/** 공격 1종의 모션, 수치, 타격 방식을 묶은 데이터임. AttackPatterns 배열 순서대로 순환함 */
USTRUCT(BlueprintType)
struct FEnemyAttackPattern
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FName Name = TEXT("Attack");

	// 비어 있으면 AttackAnimation을 대신 사용함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	TObjectPtr<class UAnimSequence> Animation;

	// 타격 1회(원거리는 투사체 1발)당 피해량임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.05"))
	float Cooldown = 1.2f;

	// 공격 시작부터 첫 타격까지의 시간임. 모션의 타격 구간에 맞춤
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
	float ImpactDelay = 0.35f;

	// 한 번의 공격에서 판정하는 횟수임. 2연타, 3연사 등에 사용함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "1"))
	int32 HitCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.01", EditCondition = "HitCount > 1"))
	float HitInterval = 0.2f;

	// 원거리 전용. 타격 1회에 동시에 발사하는 투사체 수임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Ranged", meta = (ClampMin = "1"))
	int32 ProjectilesPerHit = 1;

	// 원거리 전용. 동시 발사 투사체가 퍼지는 전체 각도임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Ranged", meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "ProjectilesPerHit > 1"))
	float SpreadAngle = 0.0f;

	// 근접 전용. 0이면 넉백하지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Melee", meta = (ClampMin = "0.0"))
	float KnockbackStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Melee", meta = (ClampMin = "0.0"))
	float KnockbackLift = 0.0f;
};

UCLASS(Blueprintable)
class UT1_API AEnemyCharacter : public AUT1Entity
{
	GENERATED_BODY()

public:
	AEnemyCharacter();
	virtual void PostLoad() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;
	virtual float TakeDamage(float DamageAmount, const struct FDamageEvent& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<class UBlendSpace> LocomotionAnimation;

	// 애니메이션이 비어 있는 공격 패턴이 대신 사용하는 기본 공격 모션임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<class UAnimSequence> AttackAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack", meta = (TitleProperty = "Name"))
	TArray<FEnemyAttackPattern> AttackPatterns;

	// 예전 BP에 저장된 값을 읽기 위해서만 남겨 둠. 로드 시 AttackPatterns로 옮기고 비움
	UPROPERTY()
	TArray<TObjectPtr<class UAnimSequence>> AttackAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<class UAnimSequence> DeathAnimation;

	float GetAttackDuration() const;
	float GetAttackCooldownRemaining() const;
	float GetChaseAcceptanceRadius() const;
	FText GetCombatTypeText() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	EEnemyCombatType CombatType = EEnemyCombatType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float DetectionRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float LoseTargetRange = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float AttackRange = 150.0f; // Horizontal reach beyond the characters' collision capsules.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Debug")
	bool bShowAttackDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float PatrolRadius = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float WalkSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float ChaseSpeed = 380.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Health")
	float CurrentHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Health")
	bool bDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "0.0"))
	float DeathCleanupDelay = 1.0f;

	// Non-zero only on the dedicated PYW validation actor.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Enemy|Debug", meta = (ClampMin = "0.0"))
	float TestDeathDelay = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Debug")
	EEnemyAIState CurrentState = EEnemyAIState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Debug")
	int32 SuccessfulAttackCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Debug")
	FString ActiveAttackName = TEXT("Default");

	void SetAIState(EEnemyAIState NewState);

	bool IsTargetInAttackRange(const AActor* Target) const;

	/** 공격 모션과 타격 판정이 모두 끝나기 전까지 true임 */
	bool IsAttackInProgress() const;

	/** 이동 회전 속도로 대상을 향해 돌아섬. 공격 모션 중에는 방향을 고정함 */
	void FaceTarget(const AActor* Target, float DeltaSeconds);

private:
	double NextAttackTime = 0.0;
	double AttackAnimationEndTime = 0.0;
	bool bPlayingAttackAnimation = false;
	int32 NextAttackPatternIndex = 0;
	FTimerHandle AttackImpactTimer;

	// 공격 도중 AttackPatterns가 바뀌어도 진행 중인 공격이 흔들리지 않도록 복사본을 씀
	UPROPERTY(Transient)
	FEnemyAttackPattern ActivePattern;

	void PlayLocomotionAnimation();
	void SelectNextAttackPattern();
	void MigrateLegacyAttackAnimations();
	void ResolveAttackImpact(TWeakObjectPtr<AActor> WeakTarget, int32 HitIndex);
	void Die(AController* Killer, AActor* DamageCauser);

protected:
	/** 타격 1회를 처리함. HitIndex는 HitCount 중 몇 번째 판정인지임 */
	virtual bool ExecuteCombatAttack(AActor* Target, const FEnemyAttackPattern& Pattern, int32 HitIndex);

public:

	UFUNCTION(BlueprintCallable, Category = "Enemy|Attack")
	bool PerformAttack(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Attack", meta = (DisplayName = "On Enemy Attack"))
	void BP_OnAttack(AActor* Target);
};

