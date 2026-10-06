#include "PYW/Entities/RangedEnemyCharacter.h"

#include "Animation/AnimSequence.h"
#include "Engine/World.h"
#include "PYW/Weapons/EnemyProjectile.h"
#include "UObject/ConstructorHelpers.h"

ARangedEnemyCharacter::ARangedEnemyCharacter()
{
	CombatType = EEnemyCombatType::Ranged;
	MaxHealth = 50.0f;
	AttackRange = 900.0f;
	WalkSpeed = 160.0f;
	ChaseSpeed = 320.0f;
	StrafeSpeed = 230.0f;
	CombatMovement = EEnemyCombatMovement::Kite;
	RetreatDistance = 320.0f;
	PoiseThreshold = 20.0f;
	ReactionTime = 0.6f;
	ProjectileClass = AEnemyProjectile::StaticClass();

	// 총을 쏘는 자세 대신 양손을 가슴 앞 중앙에 모았다가 앞으로 밀어 내는 시전 동작을 씀.
	// 맨손 시전 클립이 없어서 권총 꺼내기(MM_Pistol_Equip) 중 0.70~1.40s 구간만 잘라 씀:
	// 허리 양옆의 손이 0.95s에 가슴 앞에서 모이고(간격 9cm), 1.10s 무렵 앞으로 밀려 나감 (구간 시작 기준 0.25s / 0.40s)
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CastAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Equip"));
	constexpr float CastStart = 0.70f;
	constexpr float CastRelease = 0.40f;
	AttackAnimation = CastAsset.Object;
	AttackPatterns.Reset();

	FEnemyAttackPattern& MagicBolt = AttackPatterns.AddDefaulted_GetRef();
	MagicBolt.Name = TEXT("RangedMagicBolt");
	MagicBolt.Animation = CastAsset.Object;
	MagicBolt.AnimationStartTime = CastStart;
	MagicBolt.Damage = 4.2f;
	MagicBolt.Cooldown = 0.6f;
	MagicBolt.ImpactDelay = CastRelease;
	MagicBolt.Weight = 3.0f;

	// 손을 모은 채 앞으로 민 자세를 유지하며 연사함. 마지막 발사 후 0.25초 더 자세를 유지함
	FEnemyAttackPattern& TripleBurst = AttackPatterns.AddDefaulted_GetRef();
	TripleBurst.Name = TEXT("RangedTripleBurst");
	TripleBurst.Animation = CastAsset.Object;
	TripleBurst.AnimationStartTime = CastStart;
	TripleBurst.AnimationDuration = 1.0f;
	TripleBurst.Damage = 2.1f;
	TripleBurst.Cooldown = 0.8f;
	TripleBurst.ImpactDelay = CastRelease;
	TripleBurst.HitCount = 3;
	TripleBurst.HitInterval = 0.18f;
	TripleBurst.Weight = 2.0f;

	// 손을 밀어 내는 순간 부채꼴로 일제 사격을 함
	FEnemyAttackPattern& SpreadVolley = AttackPatterns.AddDefaulted_GetRef();
	SpreadVolley.Name = TEXT("RangedSpreadVolley");
	SpreadVolley.Animation = CastAsset.Object;
	SpreadVolley.AnimationStartTime = CastStart;
	SpreadVolley.AnimationDuration = 0.8f;
	SpreadVolley.Damage = 1.8f;
	SpreadVolley.Cooldown = 1.0f;
	SpreadVolley.ImpactDelay = CastRelease + 0.05f;
	SpreadVolley.ProjectilesPerHit = 5;
	SpreadVolley.SpreadAngle = 24.0f;
	SpreadVolley.PatternCooldown = 3.0f;
	SpreadVolley.Weight = 1.5f;

	// 붙어 오는 대상을 밀쳐 내고 다시 거리를 벌리기 위한 근거리 폭발임. 모은 손을 밀어 내는 순간 터짐
	FEnemyAttackPattern& RepelNova = AttackPatterns.AddDefaulted_GetRef();
	RepelNova.Name = TEXT("RangedRepelNova");
	RepelNova.Animation = CastAsset.Object;
	RepelNova.AnimationStartTime = CastStart;
	RepelNova.Damage = 2.8f;
	RepelNova.Cooldown = 0.6f;
	RepelNova.ImpactDelay = CastRelease;
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
	const FVector LaunchOrigin = GetProjectileOrigin();
	const FVector Direction = (AimPoint - LaunchOrigin).GetSafeNormal().RotateAngleAxis(YawOffsetDegrees, FVector::UpVector);
	// 손끝보다 조금 앞에서 생성해 팔·몸 메시와 겹쳐 보이지 않게 함
	const FTransform SpawnTransform(Direction.Rotation(), LaunchOrigin + Direction * 25.0f);
	AEnemyProjectile* Projectile = GetWorld()->SpawnActorDeferred<AEnemyProjectile>(
		ProjectileClass, SpawnTransform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile) return false;
	// 치명타는 한 발씩 따로 굴림. 부채꼴 사격은 일부 탄만 치명타가 될 수 있음
	float ShotDamage = Damage;
	Projectile->DamageTypeClass = RollDamageType(ShotDamage);
	Projectile->Damage = ShotDamage;
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
