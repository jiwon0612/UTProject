#include "PYW/RangedEnemyCharacter.h"

#include "Animation/AnimSequence.h"
#include "Engine/World.h"
#include "PYW/EnemyProjectile.h"
#include "UObject/ConstructorHelpers.h"

ARangedEnemyCharacter::ARangedEnemyCharacter()
{
	CombatType = EEnemyCombatType::Ranged;
	AttackRange = 900.0f;
	AttackDamage = 12.0f;
	AttackCooldown = 1.8f;
	WalkSpeed = 160.0f;
	ChaseSpeed = 320.0f;
	ProjectileClass = AEnemyProjectile::StaticClass();

	static ConstructorHelpers::FObjectFinder<UAnimSequence> RangedAttackAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack"));
	if (RangedAttackAsset.Succeeded())
	{
		AttackAnimation = RangedAttackAsset.Object;
		AttackAnimations.Reset();
		AttackAnimations.Add(RangedAttackAsset.Object);
	}
}

bool ARangedEnemyCharacter::ExecuteCombatAttack(AActor* Target)
{
	if (!IsValid(Target) || !ProjectileClass) return false;

	const FVector AimPoint = Target->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
	const FVector LaunchOrigin = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const FVector Direction = (AimPoint - LaunchOrigin).GetSafeNormal();
	const FTransform SpawnTransform(Direction.Rotation(), LaunchOrigin + Direction * 70.0f);
	AEnemyProjectile* Projectile = GetWorld()->SpawnActorDeferred<AEnemyProjectile>(
		ProjectileClass, SpawnTransform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile) return false;
	Projectile->Damage = AttackDamage;
	Projectile->Speed = ProjectileSpeed;
	Projectile->FinishSpawning(SpawnTransform);
	UE_LOG(LogTemp, Display, TEXT("ENEMY_RANGED ProjectileSpawned Enemy=%s Target=%s"), *GetName(), *GetNameSafe(Target));
	return true;
}
