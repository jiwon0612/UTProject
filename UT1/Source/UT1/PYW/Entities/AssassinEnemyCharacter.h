#pragma once

#include "CoreMinimal.h"
#include "PYW/Entities/MeleeEnemyCharacter.h"
#include "AssassinEnemyCharacter.generated.h"

/** Fragile skirmisher: reacts fast, circles quickly, darts in with stabs, flurries and lunges. */
UCLASS(Blueprintable)
class UT1_API AAssassinEnemyCharacter : public AMeleeEnemyCharacter
{
	GENERATED_BODY()

public:
	AAssassinEnemyCharacter();
};
