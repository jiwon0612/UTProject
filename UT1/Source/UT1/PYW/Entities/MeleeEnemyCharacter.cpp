#include "PYW/Entities/MeleeEnemyCharacter.h"

#include "Animation/AnimSequence.h"
#include "UObject/ConstructorHelpers.h"

AMeleeEnemyCharacter::AMeleeEnemyCharacter()
{
	CombatType = EEnemyCombatType::Melee;
	CombatMovement = EEnemyCombatMovement::Strafe;
	AttackRange = 120.0f;
	WalkSpeed = 170.0f;
	ChaseSpeed = 400.0f;
	StrafeSpeed = 170.0f;
	PoiseThreshold = 30.0f;
	StaggerDuration = 0.55f;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack01(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack02(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack03(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03"));
	AttackAnimation = Attack01.Object;

	// 앞의 세 패턴은 AN_MeleeAttack_01~03 순서와 맞춤. setup 스크립트가 순서대로 애니메이션을 교체함
	// ImpactDelay는 MM_Attack_01~03(1.00s/1.00s/1.67s) 모션의 타격 구간 기준임
	AttackPatterns.Reset();

	FEnemyAttackPattern& Slash = AttackPatterns.AddDefaulted_GetRef();
	Slash.Name = TEXT("MeleeSlash");
	Slash.Animation = Attack01.Object;
	Slash.Damage = 18.0f;
	Slash.Cooldown = 0.5f;
	Slash.ImpactDelay = 0.35f;
	Slash.Weight = 3.0f;

	FEnemyAttackPattern& DoubleStrike = AttackPatterns.AddDefaulted_GetRef();
	DoubleStrike.Name = TEXT("MeleeDoubleStrike");
	DoubleStrike.Animation = Attack02.Object;
	DoubleStrike.Damage = 11.0f;
	DoubleStrike.Cooldown = 0.7f;
	DoubleStrike.ImpactDelay = 0.3f;
	DoubleStrike.HitCount = 2;
	DoubleStrike.HitInterval = 0.22f;
	DoubleStrike.Weight = 2.0f;

	FEnemyAttackPattern& HeavySmash = AttackPatterns.AddDefaulted_GetRef();
	HeavySmash.Name = TEXT("MeleeHeavySmash");
	HeavySmash.Animation = Attack03.Object;
	HeavySmash.Damage = 34.0f;
	HeavySmash.Cooldown = 0.9f;
	HeavySmash.ImpactDelay = 0.65f;
	HeavySmash.KnockbackStrength = 420.0f;
	HeavySmash.KnockbackLift = 110.0f;
	HeavySmash.PatternCooldown = 4.0f;
	HeavySmash.bSuperArmor = true;

	FEnemyAttackPattern& LungeSlash = AttackPatterns.AddDefaulted_GetRef();
	LungeSlash.Name = TEXT("MeleeLungeSlash");
	LungeSlash.Animation = Attack01.Object;
	LungeSlash.Damage = 16.0f;
	LungeSlash.Cooldown = 0.6f;
	LungeSlash.ImpactDelay = 0.6f;
	LungeSlash.MinRange = 150.0f;
	LungeSlash.MaxRange = 360.0f;
	LungeSlash.LungeDelay = 0.25f;
	LungeSlash.LungeSpeed = 1200.0f;
	LungeSlash.LungeLift = 150.0f;
	LungeSlash.PatternCooldown = 5.0f;
	LungeSlash.Weight = 2.0f;
}

bool AMeleeEnemyCharacter::ExecuteCombatAttack(AActor* Target, const FEnemyAttackPattern& Pattern, int32 HitIndex)
{
	return ApplyStrikeHit(Target, Pattern, StrikeHalfAngle);
}
