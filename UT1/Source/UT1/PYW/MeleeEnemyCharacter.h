#pragma once

#include "CoreMinimal.h"
#include "PYW/EnemyCharacter.h"
#include "MeleeEnemyCharacter.generated.h"

/** Medieval melee enemy: closes distance and deals damage directly. */
UCLASS(Blueprintable)
class UT1_API AMeleeEnemyCharacter : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	AMeleeEnemyCharacter();
};
