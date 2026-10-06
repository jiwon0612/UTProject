#include "PYW/Entities/AssassinEnemyCharacter.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "UObject/ConstructorHelpers.h"

AAssassinEnemyCharacter::AAssassinEnemyCharacter()
{
	GetCapsuleComponent()->SetRelativeScale3D(FVector(0.92f));
	CombatMovement = EEnemyCombatMovement::Strafe;
	MaxHealth = 65.0f;
	AttackRange = 100.0f;
	WalkSpeed = 220.0f;
	ChaseSpeed = 560.0f;
	StrafeSpeed = 360.0f;
	DetectionRange = 1700.0f;
	ReactionTime = 0.25f;
	PoiseThreshold = 18.0f;
	StaggerDuration = 0.45f;
	StrikeHalfAngle = 60.0f;
	AttackCooldownVariance = 0.3f;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack01(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack02(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02"));
	AttackAnimation = Attack01.Object;
	AttackPatterns.Reset();

	FEnemyAttackPattern& QuickStab = AttackPatterns.AddDefaulted_GetRef();
	QuickStab.Name = TEXT("AssassinQuickStab");
	QuickStab.Animation = Attack01.Object;
	QuickStab.Damage = 9.0f;
	QuickStab.Cooldown = 0.35f;
	QuickStab.ImpactDelay = 0.22f;
	QuickStab.Weight = 3.0f;

	FEnemyAttackPattern& Flurry = AttackPatterns.AddDefaulted_GetRef();
	Flurry.Name = TEXT("AssassinFlurry");
	Flurry.Animation = Attack02.Object;
	Flurry.Damage = 6.0f;
	Flurry.Cooldown = 0.6f;
	Flurry.ImpactDelay = 0.25f;
	Flurry.HitCount = 3;
	Flurry.HitInterval = 0.14f;
	Flurry.Weight = 2.0f;

	// 선회하다가 틈을 보고 파고드는 찌르기임
	FEnemyAttackPattern& LungeStab = AttackPatterns.AddDefaulted_GetRef();
	LungeStab.Name = TEXT("AssassinLungeStab");
	LungeStab.Animation = Attack01.Object;
	LungeStab.Damage = 16.0f;
	LungeStab.Cooldown = 0.5f;
	LungeStab.ImpactDelay = 0.5f;
	LungeStab.MinRange = 180.0f;
	LungeStab.MaxRange = 450.0f;
	LungeStab.LungeDelay = 0.15f;
	LungeStab.LungeSpeed = 1400.0f;
	LungeStab.LungeLift = 160.0f;
	LungeStab.PatternCooldown = 3.5f;
	LungeStab.Weight = 3.0f;
}
