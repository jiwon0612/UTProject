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

	// 패턴마다 다른 짧은 검술 모션을 씀 (CJW hackNSlash, 읽기 전용). setup에서 인플레이스로 리타게팅함
	// QuickThrust 0.62s(0.2~0.3s에 팔을 곧게 뻗음), SpinCuts 0.82s(연달아 휘두름), LowLunge 0.75s(0.45s에 가장 멀리 파고듦)
	static ConstructorHelpers::FObjectFinder<UAnimSequence> QuickThrust(
		TEXT("/Game/CJW/Assets/hackNSlash/Animations/Combo_10/Anim_Combo_10_Br_1"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SpinCuts(
		TEXT("/Game/CJW/Assets/hackNSlash/Animations/Combo_4/Anim_Combo_4_Br_2"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> LowLunge(
		TEXT("/Game/CJW/Assets/hackNSlash/Animations/Combo_4/Anim_Combo_4_Br_1"));
	AttackAnimation = QuickThrust.Object;
	AttackPatterns.Reset();

	FEnemyAttackPattern& QuickStab = AttackPatterns.AddDefaulted_GetRef();
	QuickStab.Name = TEXT("AssassinQuickStab");
	QuickStab.Animation = QuickThrust.Object;
	QuickStab.Damage = 9.0f;
	QuickStab.Cooldown = 0.35f;
	QuickStab.ImpactDelay = 0.22f;
	QuickStab.Weight = 3.0f;

	FEnemyAttackPattern& Flurry = AttackPatterns.AddDefaulted_GetRef();
	Flurry.Name = TEXT("AssassinFlurry");
	Flurry.Animation = SpinCuts.Object;
	Flurry.Damage = 6.0f;
	Flurry.Cooldown = 0.6f;
	Flurry.ImpactDelay = 0.25f;
	Flurry.HitCount = 3;
	Flurry.HitInterval = 0.14f;
	Flurry.Weight = 2.0f;

	// 선회하다가 틈을 보고 파고드는 찌르기임
	FEnemyAttackPattern& LungeStab = AttackPatterns.AddDefaulted_GetRef();
	LungeStab.Name = TEXT("AssassinLungeStab");
	LungeStab.Animation = LowLunge.Object;
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
