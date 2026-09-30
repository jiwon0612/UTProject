#pragma once

#include "CoreMinimal.h"
#include "PYW/Entities/EnemyCharacter.h"
#include "MeleeEnemyCharacter.generated.h"

/** Medieval melee enemy: closes distance and deals damage directly. */
UCLASS(Blueprintable)
class UT1_API AMeleeEnemyCharacter : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	AMeleeEnemyCharacter();

	// 타격 시점에 정면 기준 이 각도 안에 있어야 맞음. 옆으로 돌아 피하는 회피를 허용하기 위함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Melee", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float StrikeHalfAngle = 70.0f;

protected:
	virtual bool ExecuteCombatAttack(AActor* Target, const FEnemyAttackPattern& Pattern, int32 HitIndex) override;

private:
	bool IsTargetInStrikeArc(const AActor* Target) const;
};
