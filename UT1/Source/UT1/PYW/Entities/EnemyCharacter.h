#pragma once

#include "CoreMinimal.h"
#include "CJW/Entities/UT1Entity.h"
#include "EnemyCharacter.generated.h"

// 새 상태는 뒤에만 추가함. 기존 값의 순서를 바꾸지 않기 위함
UENUM(BlueprintType)
enum class EEnemyAIState : uint8
{
	Idle,
	Walk,
	Chase,
	Attack,
	Alert,
	Reposition,
	Retreat,
	Stagger,
	Search
};

UENUM(BlueprintType)
enum class EEnemyCombatType : uint8
{
	Melee,
	Ranged
};

/** 공격 쿨다운 동안 대상 주변에서 움직이는 방식임 */
UENUM(BlueprintType)
enum class EEnemyCombatMovement : uint8
{
	HoldGround,	// 대상을 바라보며 천천히 압박함
	Strafe,		// 대상 주위를 돌며 틈을 봄
	Kite		// 너무 가까우면 물러나고, 아니면 옆으로 이동함
};

/** 공격 1종의 모션, 수치, 타격 방식을 묶은 데이터임. 지금 쓸 수 있는 패턴 중 가중치로 골라 씀 */
USTRUCT(BlueprintType)
struct FEnemyAttackPattern
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FName Name = TEXT("Attack");

	// 비어 있으면 AttackAnimation을 대신 사용함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	TObjectPtr<class UAnimSequence> Animation;

	// 0이면 애니메이션 전체 길이만큼 재생함. 조준 유지 같은 루프 모션은 이 시간만큼만 재생함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
	float AnimationDuration = 0.0f;

	// 타격 1회(원거리는 투사체 1발)당 피해량임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	// 공격이 끝난 뒤 다음 공격까지 쉬는 시간임. 이 동안 CombatMovement대로 움직임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
	float Cooldown = 0.5f;

	// 공격 시작부터 첫 타격까지의 시간임. 모션의 타격 구간에 맞춤
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
	float ImpactDelay = 0.35f;

	// 한 번의 공격에서 판정하는 횟수임. 2연타, 3연사 등에 사용함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "1"))
	int32 HitCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.01", EditCondition = "HitCount > 1"))
	float HitInterval = 0.2f;

	// 사용 가능한 패턴 중 선택될 확률 가중치임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Selection", meta = (ClampMin = "0.01"))
	float Weight = 1.0f;

	// 대상과의 간격(캡슐 표면 기준)이 이 범위 안일 때만 사용함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Selection", meta = (ClampMin = "0.0"))
	float MinRange = 0.0f;

	// 0이면 캐릭터의 AttackRange를 사용함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Selection", meta = (ClampMin = "0.0"))
	float MaxRange = 0.0f;

	// 이 패턴만 따로 다시 쓰기까지의 시간임. 돌진 같은 큰 기술을 연달아 쓰지 않게 함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Selection", meta = (ClampMin = "0.0"))
	float PatternCooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Selection")
	bool bEnragedOnly = false;

	// 공격 도중 피해를 받아도 경직되지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Selection")
	bool bSuperArmor = false;

	// 공격 시작 후 이 시간이 지나면 대상 쪽으로 튀어 나감
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Lunge", meta = (ClampMin = "0.0"))
	float LungeDelay = 0.0f;

	// 수평 최대 속도임. 대상 앞에 착지하도록 체공 시간에 맞춰 줄어듦
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Lunge", meta = (ClampMin = "0.0"))
	float LungeSpeed = 0.0f;

	// 지면 마찰로 돌진이 바로 멈추지 않도록 띄우는 속도임. 크게 주면 도약 공격이 됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Lunge", meta = (ClampMin = "0.0"))
	float LungeLift = 0.0f;

	// 0보다 크면 방향과 무관하게 자신 주변 원형 범위로 판정함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Hit", meta = (ClampMin = "0.0"))
	float AreaRadius = 0.0f;

	// 범위 판정 중심을 자신 대신 공격 시작 순간의 대상 발밑으로 둠 (포격). 그 뒤 대상이 움직여도 중심은 따라가지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Hit", meta = (EditCondition = "AreaRadius > 0"))
	bool bAreaAtTarget = false;

	// bAreaAtTarget 연타 전용. 두 번째 판정부터 중심을 대상 발밑에서 이 반경 안으로 흩뿌림
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Hit", meta = (ClampMin = "0.0", EditCondition = "bAreaAtTarget"))
	float AreaScatter = 0.0f;

	// 원거리 전용. 타격 1회에 동시에 발사하는 투사체 수임. AreaRadius가 있으면 무시함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Hit", meta = (ClampMin = "1"))
	int32 ProjectilesPerHit = 1;

	// 원거리 전용. 동시 발사 투사체가 퍼지는 전체 각도임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Hit", meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "ProjectilesPerHit > 1"))
	float SpreadAngle = 0.0f;

	// 0이면 넉백하지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Hit", meta = (ClampMin = "0.0"))
	float KnockbackStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Hit", meta = (ClampMin = "0.0"))
	float KnockbackLift = 0.0f;

	// 마지막 타격 후 자신도 사망함 (자폭)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Hit")
	bool bConsumesSelf = false;

	// 타격 판정마다 터뜨리는 이펙트임. 범위 공격은 자기 위치, 단일 타격은 맞은 대상 위치에 생성함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Effect")
	TObjectPtr<class UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Effect", meta = (ClampMin = "0.01"))
	float ImpactEffectScale = 1.0f;

	// 발사·비행·폭발 이미터를 함께 담은 팩 시스템에서 이 공격에 맞지 않는 이미터를 끔
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Effect")
	TArray<FName> ImpactDisabledEmitters;
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

	// 죽을 때 재료와 무기 설계도를 떨굼. 드랍 규칙은 CJW 컴포넌트가 맡고, 적 종류별 드랍표는 BP에서 채움
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Loot")
	TObjectPtr<class UUT1LootDropComponent> LootDrop;

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

	// 정면에서 맞고 죽을 때와, 방향별 모션이 비어 있을 때 사용함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<class UAnimSequence> DeathAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<class UAnimSequence> DeathBackAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<class UAnimSequence> DeathLeftAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<class UAnimSequence> DeathRightAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<class UAnimSequence> HitReactAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<class UAnimSequence> HitReactBackAnimation;

	float GetAttackCooldownRemaining() const;
	float GetChaseAcceptanceRadius() const;
	FText GetCombatTypeText() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	EEnemyCombatType CombatType = EEnemyCombatType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	EEnemyCombatMovement CombatMovement = EEnemyCombatMovement::HoldGround;

	// Kite 전용. 대상과의 간격이 이보다 가까우면 뒤로 물러남
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float RetreatDistance = 0.0f;

	// AttackRange에 이 값을 더한 거리 안이면 쿨다운 동안 추격 대신 거리 조절을 함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float CombatBandPadding = 250.0f;

	// 공격 간격이 기계적으로 보이지 않도록 쿨다운을 ±비율만큼 흔듦
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float AttackCooldownVariance = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float DetectionRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float LoseTargetRange = 2200.0f;

	// 대상이 이 시간 이상 시야에서 사라지면 추적을 멈추고 마지막 위치를 수색함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float LoseSightTime = 3.0f;

	// 대상을 발견한 뒤 경계하며 멈춰 있는 시간임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float ReactionTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float SearchDuration = 4.0f;

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

	// 거리 조절(Reposition/Retreat) 중 이동 속도임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float StrafeSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "0.0"))
	float DeathCleanupDelay = 1.0f;

	// PoiseRecoveryTime 안에 이만큼 피해가 쌓이면 경직됨. 0이면 경직되지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Poise", meta = (ClampMin = "0.0"))
	float PoiseThreshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Poise", meta = (ClampMin = "0.1"))
	float PoiseRecoveryTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Poise", meta = (ClampMin = "0.1"))
	float StaggerDuration = 0.6f;

	// 체력 비율이 이 값 이하가 되면 격노함. 0이면 격노하지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Enrage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EnrageHealthRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Enrage", meta = (ClampMin = "1.0"))
	float EnrageSpeedMultiplier = 1.3f;

	// 격노 중 공격 쿨다운에 곱하는 값임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Enrage", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float EnrageCooldownMultiplier = 0.7f;

	// Non-zero only on the dedicated PYW validation actor.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Enemy|Debug", meta = (ClampMin = "0.0"))
	float TestDeathDelay = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Debug")
	EEnemyAIState CurrentState = EEnemyAIState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Debug")
	bool bEnraged = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Debug")
	int32 SuccessfulAttackCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Debug")
	FString ActiveAttackName = TEXT("Default");

	void SetAIState(EEnemyAIState NewState);

	bool IsTargetInAttackRange(const AActor* Target) const;

	/** 추적·공격해도 되는 대상인지임. 죽은 Entity는 제외해서 시체를 계속 때리지 않게 함 */
	static bool IsValidCombatTarget(const AActor* Target);

	/** 두 캡슐 표면 사이의 수평 간격임. 높이 차가 커서 닿을 수 없으면 최대값을 돌려줌 */
	float GetTargetGap(const AActor* Target) const;

	/** 쿨다운이 끝났고 지금 거리에서 쓸 수 있는 패턴이 있으면 true임 */
	bool CanStartAttack(const AActor* Target) const;

	/** 쿨다운 동안 추격하지 않고 거리 조절을 할 만큼 가까우면 true임 */
	bool IsInCombatBand(const AActor* Target) const;

	/** 공격 모션, 돌진, 타격 판정이 하나라도 남아 있으면 true임 */
	bool IsAttackInProgress() const;

	bool IsStaggered() const;
	bool IsAlerting() const;

	/** 대상을 새로 발견했을 때 ReactionTime 동안 경계 상태로 둠 */
	void BeginAlert();

	void MarkTargetSeen();
	float GetTimeSinceTargetSeen() const;

	/** 이동 회전 속도로 대상을 향해 돌아섬. 공격 중에는 방향을 고정함 */
	void FaceTarget(const AActor* Target, float DeltaSeconds);

	void TurnTowardsYaw(float TargetYaw, float DeltaSeconds, float DegreesPerSecond);

	float GetMovementSpeedForState(EEnemyAIState State) const;

private:
	double NextAttackTime = 0.0;
	double ActionAnimationEndTime = 0.0;
	double StaggerEndTime = 0.0;
	double AlertEndTime = 0.0;
	double LastTargetSeenTime = 0.0;
	double LastPoiseDamageTime = 0.0;
	float AccumulatedPoiseDamage = 0.0f;
	bool bPlayingActionAnimation = false;
	int32 LastPatternIndex = INDEX_NONE;
	TArray<double> PatternReadyTimes;
	FTimerHandle AttackImpactTimer;
	FTimerHandle LungeTimer;

	// 공격 도중 AttackPatterns가 바뀌어도 진행 중인 공격이 흔들리지 않도록 복사본을 씀
	UPROPERTY(Transient)
	FEnemyAttackPattern ActivePattern;

	// bAreaAtTarget 패턴의 타격별 범위 중심(바닥 높이)임. 공격 시작 때 정해서 미리 표시함
	TArray<FVector> ActiveAreaCenters;

	void PlayLocomotionAnimation();
	void PlayActionAnimation(class UAnimSequence* Animation, float MaxDuration = 0.0f);
	bool IsPatternUsable(int32 PatternIndex, float Gap) const;
	int32 ChooseAttackPattern(const AActor* Target) const;
	float GetAttackRecoveryTime() const;
	void MigrateLegacyAttackAnimations();
	void ResolveAttackImpact(TWeakObjectPtr<AActor> WeakTarget, int32 HitIndex);
	void PerformLunge(TWeakObjectPtr<AActor> WeakTarget);
	void CancelActiveAttack();
	void Stagger(const AActor* Source);
	void Enrage();
	class UAnimSequence* SelectDirectionalAnimation(const AActor* Source, class UAnimSequence* Front,
		class UAnimSequence* Back, class UAnimSequence* Left, class UAnimSequence* Right) const;
	FVector GetAreaCenter(int32 HitIndex) const;

protected:
	void ShowDebugText(const FString& Text, const FColor& Color);

	/** Entity가 공통 체력 차감을 끝낸 뒤, 적 전용 피격 반응을 처리함. */
	virtual void HandleDamaged(float ActualDamage, AActor* DamageCauser) override;

	/** Entity의 단일 사망 판정 지점. AI와 적 전용 사망 연출을 여기서 정리함. */
	virtual void HandleDeath(AActor* Killer) override;

	/** 타격 1회를 처리함. HitIndex는 HitCount 중 몇 번째 판정인지임 */
	virtual bool ExecuteCombatAttack(AActor* Target, const FEnemyAttackPattern& Pattern, int32 HitIndex);

	/** 근접/범위 타격 공통 처리임. AreaRadius가 있으면 원형, 없으면 사거리와 정면 각도로 판정함 */
	bool ApplyStrikeHit(AActor* Target, const FEnemyAttackPattern& Pattern, float HalfAngleDegrees, int32 HitIndex = 0);

public:

	UFUNCTION(BlueprintCallable, Category = "Enemy|Attack")
	bool PerformAttack(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Attack", meta = (DisplayName = "On Enemy Attack"))
	void BP_OnAttack(AActor* Target);
};

