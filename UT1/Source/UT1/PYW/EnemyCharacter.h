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

UCLASS(Blueprintable)
class UT1_API AEnemyCharacter : public AUT1Entity
{
	GENERATED_BODY()

public:
	AEnemyCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<class UBlendSpace> LocomotionAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<class UAnimSequence> AttackAnimation;

	float GetAttackDuration() const;
	float GetAttackCooldownRemaining() const;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack", meta = (ClampMin = "0.0"))
	float AttackDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack", meta = (ClampMin = "0.05"))
	float AttackCooldown = 1.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Debug")
	EEnemyAIState CurrentState = EEnemyAIState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Debug")
	int32 SuccessfulAttackCount = 0;

	void SetAIState(EEnemyAIState NewState);

	bool IsTargetInAttackRange(const AActor* Target) const;

private:
	double NextAttackTime = 0.0;
	double AttackAnimationEndTime = 0.0;
	bool bPlayingAttackAnimation = false;
	void PlayLocomotionAnimation();

public:

	UFUNCTION(BlueprintCallable, Category = "Enemy|Attack")
	bool PerformAttack(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Attack", meta = (DisplayName = "On Enemy Attack"))
	void BP_OnAttack(AActor* Target);
};

