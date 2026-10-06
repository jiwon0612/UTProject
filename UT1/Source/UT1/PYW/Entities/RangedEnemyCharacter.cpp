#include "PYW/Entities/RangedEnemyCharacter.h"

#include "Animation/AnimSequence.h"
#include "Engine/World.h"
#include "PYW/Gameplay/EnemyProjectile.h"
#include "UObject/ConstructorHelpers.h"

ARangedEnemyCharacter::ARangedEnemyCharacter()
{
	CombatType = EEnemyCombatType::Ranged;
	AttackRange = 900.0f;
	WalkSpeed = 160.0f;
	ChaseSpeed = 320.0f;
	StrafeSpeed = 230.0f;
	CombatMovement = EEnemyCombatMovement::Kite;
	RetreatDistance = 320.0f;
	PoiseThreshold = 20.0f;
	ReactionTime = 0.6f;
	ProjectileClass = AEnemyProjectile::StaticClass();

	static ConstructorHelpers::FObjectFinder<UAnimSequence> QuickCastAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RangedAttackAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> PowerCastAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03"));
	AttackAnimation = QuickCastAsset.Object;

	// 앞의 세 패턴은 AN_RangedQuickCast/Cast/PowerCast 순서와 맞춤. setup 스크립트가 순서대로 애니메이션을 교체함
	// ImpactDelay는 QuickCast/Cast/PowerCast(1.00s/1.83s/1.67s) 모션의 발사 구간 기준임
	AttackPatterns.Reset();

	FEnemyAttackPattern& MagicBolt = AttackPatterns.AddDefaulted_GetRef();
	MagicBolt.Name = TEXT("RangedMagicBolt");
	MagicBolt.Animation = QuickCastAsset.Object;
	MagicBolt.Damage = 12.0f;
	MagicBolt.Cooldown = 0.6f;
	MagicBolt.ImpactDelay = 0.35f;
	MagicBolt.Weight = 3.0f;

	FEnemyAttackPattern& TripleBurst = AttackPatterns.AddDefaulted_GetRef();
	TripleBurst.Name = TEXT("RangedTripleBurst");
	TripleBurst.Animation = RangedAttackAsset.Object;
	TripleBurst.Damage = 6.0f;
	TripleBurst.Cooldown = 0.8f;
	TripleBurst.ImpactDelay = 0.7f;
	TripleBurst.HitCount = 3;
	TripleBurst.HitInterval = 0.18f;
	TripleBurst.Weight = 2.0f;

	FEnemyAttackPattern& SpreadVolley = AttackPatterns.AddDefaulted_GetRef();
	SpreadVolley.Name = TEXT("RangedSpreadVolley");
	SpreadVolley.Animation = PowerCastAsset.Object;
	SpreadVolley.Damage = 5.0f;
	SpreadVolley.Cooldown = 1.0f;
	SpreadVolley.ImpactDelay = 0.65f;
	SpreadVolley.ProjectilesPerHit = 5;
	SpreadVolley.SpreadAngle = 24.0f;
	SpreadVolley.PatternCooldown = 3.0f;
	SpreadVolley.Weight = 1.5f;

	// 붙어 오는 대상을 밀쳐 내고 다시 거리를 벌리기 위한 근거리 폭발임
	FEnemyAttackPattern& RepelNova = AttackPatterns.AddDefaulted_GetRef();
	RepelNova.Name = TEXT("RangedRepelNova");
	RepelNova.Animation = PowerCastAsset.Object;
	RepelNova.Damage = 8.0f;
	RepelNova.Cooldown = 0.6f;
	RepelNova.ImpactDelay = 0.45f;
	RepelNova.MaxRange = 180.0f;
	RepelNova.AreaRadius = 240.0f;
	RepelNova.KnockbackStrength = 650.0f;
	RepelNova.KnockbackLift = 150.0f;
	RepelNova.PatternCooldown = 6.0f;
	RepelNova.Weight = 4.0f;
	RepelNova.bSuperArmor = true;
}

bool ARangedEnemyCharacter::SpawnProjectileAtTarget(AActor* Target, float Damage, float YawOffsetDegrees)
{
	if (!IsValid(Target) || !ProjectileClass || bIsDead) return false;

	const FVector AimPoint = Target->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
	const FVector LaunchOrigin = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const FVector Direction = (AimPoint - LaunchOrigin).GetSafeNormal().RotateAngleAxis(YawOffsetDegrees, FVector::UpVector);
	const FTransform SpawnTransform(Direction.Rotation(), LaunchOrigin + Direction * 70.0f);
	AEnemyProjectile* Projectile = GetWorld()->SpawnActorDeferred<AEnemyProjectile>(
		ProjectileClass, SpawnTransform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile) return false;
	Projectile->Damage = Damage;
	Projectile->Speed = ProjectileSpeed;
	Projectile->FinishSpawning(SpawnTransform);
	UE_LOG(LogTemp, Display, TEXT("ENEMY_RANGED ProjectileSpawned Enemy=%s Pattern=%s Target=%s Damage=%.1f YawOffset=%.1f"),
		*GetName(), *ActiveAttackName, *GetNameSafe(Target), Damage, YawOffsetDegrees);
	return true;
}

bool ARangedEnemyCharacter::ExecuteCombatAttack(AActor* Target, const FEnemyAttackPattern& Pattern, int32 HitIndex)
{
	if (Pattern.AreaRadius > 0.0f) return ApplyStrikeHit(Target, Pattern, 180.0f);
	if (!IsValid(Target) || !ProjectileClass) return false;

	// 선딜 동안 대상이 움직였어도 발사 방향과 몸 방향이 어긋나지 않게 맞춤
	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	if (ToTarget.SizeSquared2D() > KINDA_SMALL_NUMBER) SetActorRotation(FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));

	const int32 ProjectileCount = FMath::Max(Pattern.ProjectilesPerHit, 1);
	const float Step = ProjectileCount > 1 ? Pattern.SpreadAngle / (ProjectileCount - 1) : 0.0f;
	const float FirstOffset = ProjectileCount > 1 ? -Pattern.SpreadAngle * 0.5f : 0.0f;
	bool bSpawnedAny = false;
	for (int32 ProjectileIndex = 0; ProjectileIndex < ProjectileCount; ++ProjectileIndex)
	{
		bSpawnedAny |= SpawnProjectileAtTarget(Target, Pattern.Damage, FirstOffset + Step * ProjectileIndex);
	}
	return bSpawnedAny;
}
