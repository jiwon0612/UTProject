#include "PYW/Entities/MeleeEnemyCharacter.h"

#include "Animation/AnimSequence.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AMeleeEnemyCharacter::AMeleeEnemyCharacter()
{
	CombatType = EEnemyCombatType::Melee;
	AttackRange = 120.0f;
	WalkSpeed = 170.0f;
	ChaseSpeed = 400.0f;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack01(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack02(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack03(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03"));
	AttackAnimation = Attack01.Object;

	// ImpactDelay는 MM_Attack_01~03(1.00s/1.00s/1.67s) 모션의 타격 구간 기준임
	AttackPatterns.Reset();

	FEnemyAttackPattern& Slash = AttackPatterns.AddDefaulted_GetRef();
	Slash.Name = TEXT("MeleeSlash");
	Slash.Animation = Attack01.Object;
	Slash.Damage = 18.0f;
	Slash.Cooldown = 1.1f;
	Slash.ImpactDelay = 0.35f;

	FEnemyAttackPattern& DoubleStrike = AttackPatterns.AddDefaulted_GetRef();
	DoubleStrike.Name = TEXT("MeleeDoubleStrike");
	DoubleStrike.Animation = Attack02.Object;
	DoubleStrike.Damage = 11.0f;
	DoubleStrike.Cooldown = 1.5f;
	DoubleStrike.ImpactDelay = 0.3f;
	DoubleStrike.HitCount = 2;
	DoubleStrike.HitInterval = 0.22f;

	FEnemyAttackPattern& HeavySmash = AttackPatterns.AddDefaulted_GetRef();
	HeavySmash.Name = TEXT("MeleeHeavySmash");
	HeavySmash.Animation = Attack03.Object;
	HeavySmash.Damage = 34.0f;
	HeavySmash.Cooldown = 2.0f;
	HeavySmash.ImpactDelay = 0.65f;
	HeavySmash.KnockbackStrength = 420.0f;
	HeavySmash.KnockbackLift = 110.0f;
}

bool AMeleeEnemyCharacter::IsTargetInStrikeArc(const AActor* Target) const
{
	if (!IsTargetInAttackRange(Target)) return false;
	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero()) return true;
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	return FVector::DotProduct(Forward, ToTarget) >= FMath::Cos(FMath::DegreesToRadians(StrikeHalfAngle));
}

bool AMeleeEnemyCharacter::ExecuteCombatAttack(AActor* Target, const FEnemyAttackPattern& Pattern, int32 HitIndex)
{
	if (!IsTargetInStrikeArc(Target)) return false;
	UGameplayStatics::ApplyDamage(Target, Pattern.Damage, GetController(), this, UDamageType::StaticClass());

	if (Pattern.KnockbackStrength > 0.0f || Pattern.KnockbackLift > 0.0f)
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
		{
			const FVector Knockback = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * Pattern.KnockbackStrength
				+ FVector(0.0f, 0.0f, Pattern.KnockbackLift);
			TargetCharacter->LaunchCharacter(Knockback, true, true);
			UE_LOG(LogTemp, Display, TEXT("ENEMY_MELEE Knockback Enemy=%s Target=%s Pattern=%s"),
				*GetName(), *GetNameSafe(Target), *Pattern.Name.ToString());
		}
	}
	return true;
}
