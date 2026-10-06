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

	// 맨손 시전 모션이 없어서 권총/소총 조준 자세를 무기 없이 써서 손을 뻗어 쏘는 모습으로 보이게 함.
	// 조준 자세는 8~9초 루프라 AnimationDuration으로 발사 직후까지만 재생함
	static ConstructorHelpers::FObjectFinder<UAnimSequence> OneHandAimAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> TwoHandAimAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS"));
	// 몸을 낮췄다가 팔을 크게 휘두르는 검술 모션임. 부채꼴 일제 사격의 손짓으로 씀 (0.8s 부근에서 팔을 가장 멀리 뻗음)
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SweepAsset(
		TEXT("/Game/CJW/Assets/hackNSlash/Animations/Combo_1/Anim_Combo_1_Br_3"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ChargeReleaseAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack"));
	AttackAnimation = OneHandAimAsset.Object;
	AttackPatterns.Reset();

	FEnemyAttackPattern& MagicBolt = AttackPatterns.AddDefaulted_GetRef();
	MagicBolt.Name = TEXT("RangedMagicBolt");
	MagicBolt.Animation = OneHandAimAsset.Object;
	MagicBolt.AnimationDuration = 0.75f;
	MagicBolt.Damage = 6.0f;
	MagicBolt.Cooldown = 0.6f;
	MagicBolt.ImpactDelay = 0.35f;
	MagicBolt.Weight = 3.0f;

	// 두 손 조준을 유지한 채 연사함. 마지막 발사 후 0.3초 더 자세를 유지함
	FEnemyAttackPattern& TripleBurst = AttackPatterns.AddDefaulted_GetRef();
	TripleBurst.Name = TEXT("RangedTripleBurst");
	TripleBurst.Animation = TwoHandAimAsset.Object;
	TripleBurst.AnimationDuration = 1.15f;
	TripleBurst.Damage = 3.0f;
	TripleBurst.Cooldown = 0.8f;
	TripleBurst.ImpactDelay = 0.5f;
	TripleBurst.HitCount = 3;
	TripleBurst.HitInterval = 0.18f;
	TripleBurst.Weight = 2.0f;

	// 팔을 크게 휘두르는 순간 부채꼴로 일제 사격을 함
	FEnemyAttackPattern& SpreadVolley = AttackPatterns.AddDefaulted_GetRef();
	SpreadVolley.Name = TEXT("RangedSpreadVolley");
	SpreadVolley.Animation = SweepAsset.Object;
	SpreadVolley.Damage = 2.5f;
	SpreadVolley.Cooldown = 1.0f;
	SpreadVolley.ImpactDelay = 0.8f;
	SpreadVolley.ProjectilesPerHit = 5;
	SpreadVolley.SpreadAngle = 24.0f;
	SpreadVolley.PatternCooldown = 3.0f;
	SpreadVolley.Weight = 1.5f;

	// 붙어 오는 대상을 밀쳐 내고 다시 거리를 벌리기 위한 근거리 폭발임. 힘을 모았다 터뜨리는 모션의 방출 구간에 맞춤
	FEnemyAttackPattern& RepelNova = AttackPatterns.AddDefaulted_GetRef();
	RepelNova.Name = TEXT("RangedRepelNova");
	RepelNova.Animation = ChargeReleaseAsset.Object;
	RepelNova.Damage = 4.0f;
	RepelNova.Cooldown = 0.6f;
	RepelNova.ImpactDelay = 0.7f;
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
