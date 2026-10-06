#include "PYW/Entities/BruteEnemyCharacter.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "UObject/ConstructorHelpers.h"

ABruteEnemyCharacter::ABruteEnemyCharacter()
{
	// 루트 캡슐 배율은 스폰 트랜스폼과 곱해져 유지되므로 메시와 판정 크기가 함께 커짐
	GetCapsuleComponent()->SetRelativeScale3D(FVector(1.3f));
	CombatMovement = EEnemyCombatMovement::HoldGround;
	MaxHealth = 320.0f;
	AttackRange = 140.0f;
	WalkSpeed = 140.0f;
	ChaseSpeed = 290.0f;
	StrafeSpeed = 150.0f;
	DetectionRange = 1300.0f;
	ReactionTime = 0.8f;
	PoiseThreshold = 90.0f;
	StaggerDuration = 0.8f;
	StrikeHalfAngle = 80.0f;
	EnrageHealthRatio = 0.4f;
	EnrageSpeedMultiplier = 1.35f;
	EnrageCooldownMultiplier = 0.6f;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack01(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack03(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ChargedAttack(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack"));
	// 뛰어올라 내려찍는 검술 모션임. 수평 이동은 Lunge가 맡으므로 setup에서 인플레이스로 바꿔 씀
	static ConstructorHelpers::FObjectFinder<UAnimSequence> JumpSlam(
		TEXT("/Game/CJW/Assets/hackNSlash/Animations/Combo_6/Anim_Combo_6_Br_2"));
	AttackAnimation = Attack03.Object;
	AttackPatterns.Reset();

	FEnemyAttackPattern& Smash = AttackPatterns.AddDefaulted_GetRef();
	Smash.Name = TEXT("BruteSmash");
	Smash.Animation = Attack03.Object;
	Smash.Damage = 15.0f;
	Smash.Cooldown = 0.9f;
	Smash.ImpactDelay = 0.7f;
	Smash.KnockbackStrength = 450.0f;
	Smash.KnockbackLift = 120.0f;
	Smash.Weight = 3.0f;
	Smash.bSuperArmor = true;

	// 근거리 전방위 범위 공격임. 등 뒤로 돌아가는 대상을 떼어 냄
	FEnemyAttackPattern& GroundSlam = AttackPatterns.AddDefaulted_GetRef();
	GroundSlam.Name = TEXT("BruteGroundSlam");
	GroundSlam.Animation = ChargedAttack.Object;
	GroundSlam.Damage = 12.0f;
	GroundSlam.Cooldown = 1.2f;
	GroundSlam.ImpactDelay = 0.95f;
	GroundSlam.MaxRange = 200.0f;
	GroundSlam.AreaRadius = 300.0f;
	GroundSlam.KnockbackStrength = 300.0f;
	GroundSlam.KnockbackLift = 250.0f;
	GroundSlam.PatternCooldown = 5.0f;
	GroundSlam.Weight = 2.0f;
	GroundSlam.bSuperArmor = true;

	// 중거리에서 몸을 낮췄다가 들이받는 돌진임. 선딜 동안 옆으로 빠지면 피할 수 있음
	FEnemyAttackPattern& Charge = AttackPatterns.AddDefaulted_GetRef();
	Charge.Name = TEXT("BruteCharge");
	Charge.Animation = Attack01.Object;
	Charge.Damage = 13.0f;
	Charge.Cooldown = 1.0f;
	Charge.ImpactDelay = 0.85f;
	Charge.MinRange = 250.0f;
	Charge.MaxRange = 650.0f;
	Charge.LungeDelay = 0.45f;
	Charge.LungeSpeed = 1500.0f;
	Charge.LungeLift = 220.0f;
	Charge.KnockbackStrength = 600.0f;
	Charge.KnockbackLift = 150.0f;
	Charge.PatternCooldown = 6.0f;
	Charge.Weight = 3.0f;
	Charge.bSuperArmor = true;

	FEnemyAttackPattern& LeapSlam = AttackPatterns.AddDefaulted_GetRef();
	LeapSlam.Name = TEXT("BruteLeapSlam");
	LeapSlam.Animation = JumpSlam.Object;
	LeapSlam.Damage = 16.0f;
	LeapSlam.Cooldown = 1.0f;
	LeapSlam.ImpactDelay = 1.35f;
	LeapSlam.MinRange = 150.0f;
	LeapSlam.MaxRange = 700.0f;
	LeapSlam.LungeDelay = 0.35f;
	LeapSlam.LungeSpeed = 1400.0f;
	LeapSlam.LungeLift = 480.0f;
	LeapSlam.AreaRadius = 260.0f;
	LeapSlam.KnockbackStrength = 500.0f;
	LeapSlam.KnockbackLift = 220.0f;
	LeapSlam.PatternCooldown = 5.0f;
	LeapSlam.Weight = 4.0f;
	LeapSlam.bEnragedOnly = true;
	LeapSlam.bSuperArmor = true;
}
