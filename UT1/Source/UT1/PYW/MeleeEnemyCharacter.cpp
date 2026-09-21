#include "PYW/MeleeEnemyCharacter.h"

#include "Animation/AnimSequence.h"
#include "UObject/ConstructorHelpers.h"

AMeleeEnemyCharacter::AMeleeEnemyCharacter()
{
	CombatType = EEnemyCombatType::Melee;
	AttackRange = 120.0f;
	AttackDamage = 20.0f;
	AttackCooldown = 1.2f;
	WalkSpeed = 170.0f;
	ChaseSpeed = 400.0f;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack01(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack02(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack03(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03"));
	AttackAnimations.Reset();
	if (Attack01.Succeeded()) AttackAnimations.Add(Attack01.Object);
	if (Attack02.Succeeded()) AttackAnimations.Add(Attack02.Object);
	if (Attack03.Succeeded()) AttackAnimations.Add(Attack03.Object);
	if (!AttackAnimations.IsEmpty()) AttackAnimation = AttackAnimations[0];
}
