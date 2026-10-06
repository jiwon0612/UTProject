#pragma once

#include "CoreMinimal.h"
#include "PYW/Entities/EnemyCharacter.h"
#include "ArtilleryEnemyCharacter.generated.h"

/** Artillery: keeps far away and shells marked circles at the target's feet, so the player has to keep moving. */
UCLASS(Blueprintable)
class UT1_API AArtilleryEnemyCharacter : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	AArtilleryEnemyCharacter();
};
