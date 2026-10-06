#pragma once

#include "CoreMinimal.h"
#include "PYW/Entities/MeleeEnemyCharacter.h"
#include "BruteEnemyCharacter.generated.h"

/** Heavy bruiser: slow and hard to stagger, charges from mid range, slams the ground and leaps when enraged. */
UCLASS(Blueprintable)
class UT1_API ABruteEnemyCharacter : public AMeleeEnemyCharacter
{
	GENERATED_BODY()

public:
	ABruteEnemyCharacter();
};
