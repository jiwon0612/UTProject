#pragma once

#include "CoreMinimal.h"
#include "PYW/Entities/MeleeEnemyCharacter.h"
#include "GuardianEnemyCharacter.generated.h"

/** Shield guard: blocks most frontal damage while not attacking and turns slowly, so flanking or punishing its swings is the answer. */
UCLASS(Blueprintable)
class UT1_API AGuardianEnemyCharacter : public AMeleeEnemyCharacter
{
	GENERATED_BODY()

public:
	AGuardianEnemyCharacter();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	// 피해 출처가 정면 기준 이 각도 안이면 방패로 막음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Guard", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float GuardHalfAngle = 65.0f;

	// 막았을 때 실제로 들어가는 피해 비율임. 경직 누적도 이 피해 기준이라 정면으로는 잘 끊기지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Guard", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GuardDamageMultiplier = 0.2f;

	/** 지금 Source 쪽에서 오는 공격을 막는지임. 공격 중이거나 경직 중에는 방패가 내려가 있음 */
	UFUNCTION(BlueprintPure, Category = "Enemy|Guard")
	bool IsGuardingAgainst(const AActor* Source) const;
};
