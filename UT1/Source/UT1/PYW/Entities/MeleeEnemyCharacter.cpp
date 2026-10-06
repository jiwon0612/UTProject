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
	// 낮게 몸을 뻗어 찌르는 검술 모션임. 이동은 Lunge가 맡으므로 setup에서 골반 수평 이동을 지운 인플레이스로 씀
	static ConstructorHelpers::FObjectFinder<UAnimSequence> LungeThrust(
		TEXT("/Game/CJW/Assets/hackNSlash/Animations/Combo_2/Anim_Combo_2_Br_4"));
	AttackAnimation = Attack01.Object;

	// 패턴마다 다른 모션을 씀. ImpactDelay는 각 모션의 타격 구간 기준임
	// MM_Attack_01~03(1.00s/1.00s/1.67s), Anim_Combo_2_Br_4(1.28s, 0.3~0.5s에 가장 멀리 뻗음)
	AttackPatterns.Reset();

	FEnemyAttackPattern& Slash = AttackPatterns.AddDefaulted_GetRef();
	Slash.Name = TEXT("MeleeSlash");
	Slash.Animation = Attack01.Object;
	Slash.Damage = 6.3f;
	Slash.Cooldown = 0.5f;
	Slash.ImpactDelay = 0.35f;
	Slash.Weight = 3.0f;

	FEnemyAttackPattern& DoubleStrike = AttackPatterns.AddDefaulted_GetRef();
	DoubleStrike.Name = TEXT("MeleeDoubleStrike");
	DoubleStrike.Animation = Attack02.Object;
	DoubleStrike.Damage = 3.8f;
	DoubleStrike.Cooldown = 0.7f;
	DoubleStrike.ImpactDelay = 0.3f;
	DoubleStrike.HitCount = 2;
	DoubleStrike.HitInterval = 0.22f;
	DoubleStrike.Weight = 2.0f;

	FEnemyAttackPattern& HeavySmash = AttackPatterns.AddDefaulted_GetRef();
	HeavySmash.Name = TEXT("MeleeHeavySmash");
	HeavySmash.Animation = Attack03.Object;
	HeavySmash.Damage = 11.9f;
	HeavySmash.Cooldown = 0.9f;
	HeavySmash.ImpactDelay = 0.65f;
	HeavySmash.KnockbackStrength = 420.0f;
	HeavySmash.KnockbackLift = 110.0f;
	HeavySmash.PatternCooldown = 4.0f;
	HeavySmash.bSuperArmor = true;

	FEnemyAttackPattern& LungeSlash = AttackPatterns.AddDefaulted_GetRef();
	LungeSlash.Name = TEXT("MeleeLungeSlash");
	LungeSlash.Animation = LungeThrust.Object;
	LungeSlash.Damage = 5.6f;
	LungeSlash.Cooldown = 0.6f;
	LungeSlash.ImpactDelay = 0.5f;
	LungeSlash.MinRange = 150.0f;
	LungeSlash.MaxRange = 360.0f;
	LungeSlash.LungeDelay = 0.2f;
	LungeSlash.LungeSpeed = 1200.0f;
	LungeSlash.LungeLift = 150.0f;
	LungeSlash.PatternCooldown = 5.0f;
	LungeSlash.Weight = 2.0f;
}

bool AMeleeEnemyCharacter::ExecuteCombatAttack(AActor* Target, const FEnemyAttackPattern& Pattern, int32 HitIndex)
{
	return ApplyStrikeHit(Target, Pattern, StrikeHalfAngle);
}
