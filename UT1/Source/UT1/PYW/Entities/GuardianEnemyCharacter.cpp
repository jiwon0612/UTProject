#include "PYW/Entities/GuardianEnemyCharacter.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "UObject/ConstructorHelpers.h"

AGuardianEnemyCharacter::AGuardianEnemyCharacter()
{
	GetCapsuleComponent()->SetRelativeScale3D(FVector(1.1f));
	CombatMovement = EEnemyCombatMovement::HoldGround;
	MaxHealth = 150.0f;
	AttackRange = 120.0f;
	WalkSpeed = 150.0f;
	ChaseSpeed = 300.0f;
	StrafeSpeed = 120.0f;
	ReactionTime = 0.6f;
	PoiseThreshold = 40.0f;
	StaggerDuration = 0.8f;
	StrikeHalfAngle = 60.0f;
	// 천천히 돌아서므로 옆이나 뒤로 돌아 들어갈 틈이 생김
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 160.0f, 0.0f);

	// 패턴마다 다른 검술 모션임 (CJW hackNSlash, 읽기 전용). setup에서 인플레이스로 리타게팅함
	// ShieldBash 0.70s(0.49s에 가장 멀리 뻗음), DoubleBash 0.93s(0.30s/0.75s), HeavyChop 1.17s(0.41s에 내려침),
	// ChargeThrust 1.15s(크게 당겼다가 0.98s에 찌름)
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ShieldBashAnim(
		TEXT("/Game/CJW/Assets/hackNSlash/Animations/Combo_5/Anim_Combo_5_Br_1"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DoubleBashAnim(
		TEXT("/Game/CJW/Assets/hackNSlash/Animations/Combo_8/Anim_Combo_8_Br_2"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> HeavyChopAnim(
		TEXT("/Game/CJW/Assets/hackNSlash/Animations/Combo_2/Anim_Combo_2_Br_2"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ChargeThrustAnim(
		TEXT("/Game/CJW/Assets/hackNSlash/Animations/Combo_9/Anim_Combo_9_Br_1"));
	AttackAnimation = ShieldBashAnim.Object;
	AttackPatterns.Reset();

	FEnemyAttackPattern& ShieldBash = AttackPatterns.AddDefaulted_GetRef();
	ShieldBash.Name = TEXT("GuardianShieldBash");
	ShieldBash.Animation = ShieldBashAnim.Object;
	ShieldBash.Damage = 6.0f;
	ShieldBash.Cooldown = 0.7f;
	ShieldBash.ImpactDelay = 0.48f;
	ShieldBash.KnockbackStrength = 450.0f;
	ShieldBash.KnockbackLift = 80.0f;
	ShieldBash.Weight = 3.0f;

	FEnemyAttackPattern& DoubleBash = AttackPatterns.AddDefaulted_GetRef();
	DoubleBash.Name = TEXT("GuardianDoubleBash");
	DoubleBash.Animation = DoubleBashAnim.Object;
	DoubleBash.Damage = 4.5f;
	DoubleBash.Cooldown = 0.8f;
	DoubleBash.ImpactDelay = 0.3f;
	DoubleBash.HitCount = 2;
	DoubleBash.HitInterval = 0.45f;
	DoubleBash.Weight = 2.0f;

	FEnemyAttackPattern& HeavyChop = AttackPatterns.AddDefaulted_GetRef();
	HeavyChop.Name = TEXT("GuardianHeavyChop");
	HeavyChop.Animation = HeavyChopAnim.Object;
	HeavyChop.Damage = 12.0f;
	HeavyChop.Cooldown = 1.0f;
	HeavyChop.ImpactDelay = 0.41f;
	HeavyChop.PatternCooldown = 4.0f;
	HeavyChop.Weight = 1.5f;
	HeavyChop.bSuperArmor = true;

	// 거리를 벌린 대상에게 방패를 앞세워 돌진함. 길게 당기는 동작이 피할 신호임
	FEnemyAttackPattern& ShieldCharge = AttackPatterns.AddDefaulted_GetRef();
	ShieldCharge.Name = TEXT("GuardianShieldCharge");
	ShieldCharge.Animation = ChargeThrustAnim.Object;
	ShieldCharge.Damage = 9.0f;
	ShieldCharge.Cooldown = 1.0f;
	ShieldCharge.ImpactDelay = 0.98f;
	ShieldCharge.MinRange = 200.0f;
	ShieldCharge.MaxRange = 500.0f;
	ShieldCharge.LungeDelay = 0.7f;
	ShieldCharge.LungeSpeed = 1100.0f;
	ShieldCharge.LungeLift = 140.0f;
	ShieldCharge.KnockbackStrength = 650.0f;
	ShieldCharge.KnockbackLift = 150.0f;
	ShieldCharge.PatternCooldown = 6.0f;
	ShieldCharge.Weight = 3.0f;
	ShieldCharge.bSuperArmor = true;
}

bool AGuardianEnemyCharacter::IsGuardingAgainst(const AActor* Source) const
{
	if (bIsDead || !IsValid(Source) || IsAttackInProgress() || IsStaggered()) return false;
	const FVector ToSource = (Source->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToSource.IsNearlyZero()) return false;
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(GuardHalfAngle));
	return FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), ToSource) >= MinDot;
}

float AGuardianEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	// 무기나 투사체가 출처면 그 위치로, 없으면 공격한 폰 위치로 방향을 판단함
	const AActor* Source = DamageCauser;
	if (!IsValid(Source) && EventInstigator) Source = EventInstigator->GetPawn();
	if (DamageAmount > 0.0f && Source != this && IsGuardingAgainst(Source))
	{
		UE_LOG(LogTemp, Display, TEXT("ENEMY_GUARD_BLOCK Actor=%s Source=%s Damage=%.1f->%.1f"),
			*GetName(), *GetNameSafe(Source), DamageAmount, DamageAmount * GuardDamageMultiplier);
		DamageAmount *= GuardDamageMultiplier;
		ShowDebugText(TEXT("막음"), FColor::Cyan);
	}
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}
