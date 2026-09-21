#pragma once

#include "CoreMinimal.h"
#include "PYW/EnemyCharacter.h"
#include "RangedEnemyCharacter.generated.h"

class AEnemyProjectile;

/** Medieval ranged enemy: keeps range and launches a projectile. */
UCLASS(Blueprintable)
class UT1_API ARangedEnemyCharacter : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	ARangedEnemyCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Ranged")
	TSubclassOf<AEnemyProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Ranged", meta = (ClampMin = "100.0"))
	float ProjectileSpeed = 1200.0f;

protected:
	virtual bool ExecuteCombatAttack(AActor* Target) override;
};
