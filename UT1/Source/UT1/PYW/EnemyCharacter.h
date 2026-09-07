#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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
class UT1_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float DetectionRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float LoseTargetRange = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float AttackRange = 150.0f;

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

	UFUNCTION(BlueprintCallable, Category = "Enemy|Attack")
	bool PerformAttack(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Attack", meta = (DisplayName = "On Enemy Attack"))
	void BP_OnAttack(AActor* Target);
};

