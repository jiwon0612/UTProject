#pragma once

#include "CoreMinimal.h"
#include "PYW/Entities/MeleeEnemyCharacter.h"
#include "BomberEnemyCharacter.generated.h"

/** Kamikaze: sprints at the target and self-destructs after a fuse. Staggering it cancels the fuse. */
UCLASS(Blueprintable)
class UT1_API ABomberEnemyCharacter : public AMeleeEnemyCharacter
{
	GENERATED_BODY()

public:
	ABomberEnemyCharacter();
};
