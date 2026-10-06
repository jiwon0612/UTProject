#pragma once

#include "CoreMinimal.h"
#include "PYW/Entities/EnemyCharacter.h"
#include "RangedEnemyCharacter.generated.h"

class AEnemyProjectile;

/** Medieval ranged caster: kites to keep range, fires bolts/bursts/volleys and repels close targets. */
UCLASS(Blueprintable)
class UT1_API ARangedEnemyCharacter : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	ARangedEnemyCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Ranged")
	TSubclassOf<AEnemyProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Ranged", meta = (ClampMin = "100.0"))
	float ProjectileSpeed = 900.0f; // 날아오는 방향을 눈으로 따라갈 수 있는 속도임

protected:
	virtual bool ExecuteCombatAttack(AActor* Target, const FEnemyAttackPattern& Pattern, int32 HitIndex) override;
	bool SpawnProjectileAtTarget(AActor* Target, float Damage, float YawOffsetDegrees);
};
