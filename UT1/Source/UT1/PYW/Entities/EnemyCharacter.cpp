#include "PYW/Entities/EnemyCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "BrainComponent.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "CJW/Crafting/UT1LootDropComponent.h"
#include "PYW/AI/EnemyAIController.h"
#include "PYW/Gameplay/EnemyEffects.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UBlendSpace> MovementAsset(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AttackAsset(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathAsset(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathBackAsset(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathLeftAsset(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Left_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathRightAsset(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Right_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> HitReactAsset(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Med_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> HitReactBackAsset(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Back_Med_01"));
	LocomotionAnimation = MovementAsset.Object;
	AttackAnimation = AttackAsset.Object;
	DeathAnimation = DeathAsset.Object;
	DeathBackAnimation = DeathBackAsset.Object;
	DeathLeftAnimation = DeathLeftAsset.Object;
	DeathRightAnimation = DeathRightAsset.Object;
	HitReactAnimation = HitReactAsset.Object;
	HitReactBackAnimation = HitReactBackAsset.Object;

	FEnemyAttackPattern& DefaultPattern = AttackPatterns.AddDefaulted_GetRef();
	DefaultPattern.Name = TEXT("Default");
	DefaultPattern.Animation = AttackAsset.Object;

	LootDrop = CreateDefaultSubobject<UUT1LootDropComponent>(TEXT("LootDrop"));

	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AEnemyCharacter::PostLoad()
{
	Super::PostLoad();
	MigrateLegacyAttackAnimations();
}

void AEnemyCharacter::MigrateLegacyAttackAnimations()
{
	// BP 재컴파일로 PostLoad를 거치지 않은 CDO 값이 인스턴스에 복사될 수 있어서 BeginPlay에서도 호출함
	if (AttackAnimations.IsEmpty()) return;
	const int32 Count = FMath::Min(AttackAnimations.Num(), AttackPatterns.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (AttackAnimations[Index]) AttackPatterns[Index].Animation = AttackAnimations[Index];
	}
	AttackAnimations.Reset();
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	MigrateLegacyAttackAnimations();
	if (AttackPatterns.IsEmpty())
	{
		FEnemyAttackPattern& Fallback = AttackPatterns.AddDefaulted_GetRef();
		Fallback.Name = TEXT("Default");
		Fallback.Animation = AttackAnimation;
	}
	PatternReadyTimes.SetNumZeroed(AttackPatterns.Num());
	GetCharacterMovement()->MaxWalkSpeed = GetMovementSpeedForState(CurrentState);
	PlayLocomotionAnimation();
	if (TestDeathDelay > 0.0f)
	{
		FTimerHandle TestDeathTimer;
		GetWorldTimerManager().SetTimer(TestDeathTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			UGameplayStatics::ApplyDamage(this, MaxHealth, nullptr, this, UDamageType::StaticClass());
		}), TestDeathDelay, false);
	}
}

void AEnemyCharacter::HandleDamaged(float ActualDamage, AActor* DamageCauser)
{
	Super::HandleDamaged(ActualDamage, DamageCauser);
	UE_LOG(LogTemp, Display, TEXT("ENEMY_DAMAGE Actor=%s Damage=%.1f Health=%.1f/%.1f"),
		*GetName(), ActualDamage, CurrentHealth, MaxHealth);

	// 감지 범위 밖이나 시야 밖에서 맞아도 공격자를 바로 추적함
	APawn* SourcePawn = Cast<APawn>(DamageCauser);
	if (!SourcePawn && IsValid(DamageCauser)) SourcePawn = DamageCauser->GetInstigator();
	if (IsValid(SourcePawn) && !SourcePawn->IsA<AEnemyCharacter>())
	{
		if (AEnemyAIController* AI = Cast<AEnemyAIController>(GetController()))
		{
			AI->SetTargetActor(SourcePawn);
		}
	}

	if (!bEnraged && EnrageHealthRatio > 0.0f && CurrentHealth <= MaxHealth * EnrageHealthRatio)
	{
		Enrage();
	}

	if (PoiseThreshold > 0.0f)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if (Now - LastPoiseDamageTime > PoiseRecoveryTime) AccumulatedPoiseDamage = 0.0f;
		AccumulatedPoiseDamage += ActualDamage;
		LastPoiseDamageTime = Now;
		const bool bArmored = IsAttackInProgress() && ActivePattern.bSuperArmor;
		if (AccumulatedPoiseDamage >= PoiseThreshold && !bArmored && !IsStaggered())
		{
			const AActor* Source = DamageCauser ? DamageCauser : static_cast<AActor*>(SourcePawn);
			Stagger(Source);
		}
	}
}

void AEnemyCharacter::HandleDeath(AActor* Killer)
{
	if (bIsDead) return;
	Super::HandleDeath(Killer);
	bPlayingActionAnimation = false;
	// 대기 중인 타격, 연타, 돌진 타이머가 사망 후 발동하지 않도록 정리함
	GetWorldTimerManager().ClearAllTimersForObject(this);

	if (AEnemyAIController* AI = Cast<AEnemyAIController>(GetController()))
	{
		AI->StopMovement();
		if (UBrainComponent* Brain = AI->GetBrainComponent()) Brain->StopLogic(TEXT("Enemy died"));
	}
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	const AActor* Source = Killer;
	UAnimSequence* ChosenDeath = Source && Source != this
		? SelectDirectionalAnimation(Source, DeathAnimation, DeathBackAnimation, DeathLeftAnimation, DeathRightAnimation)
		: DeathAnimation.Get();

	float DeathDuration = 0.0f;
	if (ChosenDeath)
	{
		GetMesh()->PlayAnimation(ChosenDeath, false);
		DeathDuration = ChosenDeath->GetPlayLength();
	}
	SetLifeSpan(FMath::Max(0.1f, DeathDuration + DeathCleanupDelay));
	UE_LOG(LogTemp, Display, TEXT("ENEMY_DEATH Actor=%s Animation=%s Duration=%.2f Causer=%s"),
		*GetName(), *GetNameSafe(ChosenDeath), DeathDuration, *GetNameSafe(Killer));
}

UAnimSequence* AEnemyCharacter::SelectDirectionalAnimation(const AActor* Source, UAnimSequence* Front,
	UAnimSequence* Back, UAnimSequence* Left, UAnimSequence* Right) const
{
	UAnimSequence* Result = Front;
	if (IsValid(Source))
	{
		const FVector ToSource = (Source->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), ToSource);
		const float RightDot = FVector::DotProduct(GetActorRightVector(), ToSource);
		if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
		{
			if (ForwardDot < 0.0f && Back) Result = Back;
		}
		else if (RightDot > 0.0f && Right)
		{
			Result = Right;
		}
		else if (RightDot < 0.0f && Left)
		{
			Result = Left;
		}
	}
	return Result ? Result : Front;
}

void AEnemyCharacter::PlayLocomotionAnimation()
{
	if (LocomotionAnimation) GetMesh()->PlayAnimation(LocomotionAnimation, true);
	bPlayingActionAnimation = false;
}

void AEnemyCharacter::PlayActionAnimation(UAnimSequence* Animation, float MaxDuration)
{
	if (!Animation) return;
	GetMesh()->PlayAnimation(Animation, false);
	bPlayingActionAnimation = true;
	float Duration = Animation->GetPlayLength();
	if (MaxDuration > 0.0f) Duration = FMath::Min(Duration, MaxDuration);
	ActionAnimationEndTime = GetWorld()->GetTimeSeconds() + Duration;
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bIsDead) return;
	if (bPlayingActionAnimation && GetWorld()->GetTimeSeconds() >= ActionAnimationEndTime)
	{
		PlayLocomotionAnimation();
	}
	if (!bPlayingActionAnimation)
	{
		if (UAnimSingleNodeInstance* Animation = GetMesh()->GetSingleNodeInstance())
		{
			// BS_Idle_Walk_Run: X = local direction (-180..180), Y = speed (0..600).
			const FVector LocalVelocity = FRotator(0.0f, GetActorRotation().Yaw, 0.0f).UnrotateVector(GetVelocity());
			const float Speed = GetVelocity().Size2D();
			const float Direction = Speed > 1.0f
				? FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X)) : 0.0f;
			Animation->SetBlendSpacePosition(FVector(Direction, Speed, 0.0f));
		}
	}
}

void AEnemyCharacter::Destroyed()
{
	if (bIsDead) UE_LOG(LogTemp, Display, TEXT("ENEMY_DEATH_REMOVED Actor=%s"), *GetName());
	Super::Destroyed();
}

float AEnemyCharacter::GetTargetGap(const AActor* Target) const
{
	if (!IsValid(Target) || Target == this) return TNumericLimits<float>::Max();
	const ACharacter* TargetCharacter = Cast<ACharacter>(Target);
	const UCapsuleComponent* TargetCapsule = TargetCharacter ? TargetCharacter->GetCapsuleComponent() : nullptr;
	const float TargetRadius = TargetCapsule ? TargetCapsule->GetScaledCapsuleRadius() : 0.0f;
	const float TargetHalfHeight = TargetCapsule ? TargetCapsule->GetScaledCapsuleHalfHeight() : 0.0f;
	const FVector Offset = Target->GetActorLocation() - GetActorLocation();
	if (FMath::Abs(Offset.Z) > GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + TargetHalfHeight)
	{
		return TNumericLimits<float>::Max();
	}
	return FMath::Max(0.0f, static_cast<float>(Offset.Size2D()) - GetCapsuleComponent()->GetScaledCapsuleRadius() - TargetRadius);
}

bool AEnemyCharacter::IsTargetInAttackRange(const AActor* Target) const
{
	return GetTargetGap(Target) <= FMath::Max(AttackRange, 0.0f);
}

bool AEnemyCharacter::IsInCombatBand(const AActor* Target) const
{
	return GetTargetGap(Target) <= FMath::Max(AttackRange, 0.0f) + CombatBandPadding;
}

bool AEnemyCharacter::IsPatternUsable(int32 PatternIndex, float Gap) const
{
	if (!AttackPatterns.IsValidIndex(PatternIndex)) return false;
	const FEnemyAttackPattern& Pattern = AttackPatterns[PatternIndex];
	if (Pattern.bEnragedOnly && !bEnraged) return false;
	if (PatternReadyTimes.IsValidIndex(PatternIndex) && GetWorld()->GetTimeSeconds() < PatternReadyTimes[PatternIndex])
	{
		return false;
	}
	const float MaxRange = Pattern.MaxRange > 0.0f ? Pattern.MaxRange : AttackRange;
	return Gap >= Pattern.MinRange && Gap <= MaxRange;
}

int32 AEnemyCharacter::ChooseAttackPattern(const AActor* Target) const
{
	const float Gap = GetTargetGap(Target);
	// 같은 패턴이 연달아 나오지 않도록 직전 패턴의 가중치를 낮춤
	auto SelectionWeight = [this](int32 Index)
	{
		float Weight = FMath::Max(AttackPatterns[Index].Weight, 0.01f);
		if (Index == LastPatternIndex && AttackPatterns.Num() > 1) Weight *= 0.35f;
		return Weight;
	};

	float TotalWeight = 0.0f;
	int32 LastUsable = INDEX_NONE;
	for (int32 Index = 0; Index < AttackPatterns.Num(); ++Index)
	{
		if (!IsPatternUsable(Index, Gap)) continue;
		TotalWeight += SelectionWeight(Index);
		LastUsable = Index;
	}
	if (LastUsable == INDEX_NONE) return INDEX_NONE;

	float Roll = FMath::FRand() * TotalWeight;
	for (int32 Index = 0; Index < AttackPatterns.Num(); ++Index)
	{
		if (!IsPatternUsable(Index, Gap)) continue;
		Roll -= SelectionWeight(Index);
		if (Roll <= 0.0f) return Index;
	}
	return LastUsable;
}

bool AEnemyCharacter::IsValidCombatTarget(const AActor* Target)
{
	if (!IsValid(Target)) return false;
	const AUT1Entity* Entity = Cast<AUT1Entity>(Target);
	return !Entity || !Entity->IsDead();
}

bool AEnemyCharacter::CanStartAttack(const AActor* Target) const
{
	if (!IsValidCombatTarget(Target)) return false;
	if (bIsDead || IsStaggered() || IsAttackInProgress() || GetWorld()->GetTimeSeconds() < NextAttackTime) return false;
	const float Gap = GetTargetGap(Target);
	for (int32 Index = 0; Index < AttackPatterns.Num(); ++Index)
	{
		if (IsPatternUsable(Index, Gap)) return true;
	}
	return false;
}

bool AEnemyCharacter::IsAttackInProgress() const
{
	return bPlayingActionAnimation
		|| GetWorldTimerManager().IsTimerActive(AttackImpactTimer)
		|| GetWorldTimerManager().IsTimerActive(LungeTimer);
}

bool AEnemyCharacter::IsStaggered() const
{
	return !bIsDead && GetWorld()->GetTimeSeconds() < StaggerEndTime;
}

bool AEnemyCharacter::IsAlerting() const
{
	return !bIsDead && GetWorld()->GetTimeSeconds() < AlertEndTime;
}

void AEnemyCharacter::BeginAlert()
{
	if (bIsDead || ReactionTime <= 0.0f) return;
	AlertEndTime = GetWorld()->GetTimeSeconds() + ReactionTime;
	SetAIState(EEnemyAIState::Alert);
	ShowDebugText(TEXT("!"), FColor::Yellow);
}

void AEnemyCharacter::MarkTargetSeen()
{
	LastTargetSeenTime = GetWorld()->GetTimeSeconds();
}

float AEnemyCharacter::GetTimeSinceTargetSeen() const
{
	return static_cast<float>(GetWorld()->GetTimeSeconds() - LastTargetSeenTime);
}

void AEnemyCharacter::TurnTowardsYaw(float TargetYaw, float DeltaSeconds, float DegreesPerSecond)
{
	const FRotator Desired(0.0f, TargetYaw, 0.0f);
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Desired, DeltaSeconds, DegreesPerSecond));
}

void AEnemyCharacter::FaceTarget(const AActor* Target, float DeltaSeconds)
{
	if (bIsDead || !IsValid(Target) || IsAttackInProgress() || IsStaggered()) return;
	const FVector Offset = Target->GetActorLocation() - GetActorLocation();
	if (Offset.SizeSquared2D() <= KINDA_SMALL_NUMBER) return;
	TurnTowardsYaw(Offset.Rotation().Yaw, DeltaSeconds, static_cast<float>(GetCharacterMovement()->RotationRate.Yaw));
}

float AEnemyCharacter::GetMovementSpeedForState(EEnemyAIState State) const
{
	float Speed = WalkSpeed;
	switch (State)
	{
	case EEnemyAIState::Chase:
		Speed = ChaseSpeed;
		break;
	case EEnemyAIState::Reposition:
	case EEnemyAIState::Retreat:
		Speed = StrafeSpeed;
		break;
	case EEnemyAIState::Search:
		Speed = FMath::Lerp(WalkSpeed, ChaseSpeed, 0.4f);
		break;
	default:
		break;
	}
	return Speed * (bEnraged ? EnrageSpeedMultiplier : 1.0f);
}

float AEnemyCharacter::GetAttackCooldownRemaining() const
{
	return FMath::Max(0.0f, static_cast<float>(NextAttackTime - GetWorld()->GetTimeSeconds()));
}

float AEnemyCharacter::GetChaseAcceptanceRadius() const
{
	return FMath::Max(5.0f, AttackRange * (CombatType == EEnemyCombatType::Ranged ? 0.8f : 0.35f));
}

FText AEnemyCharacter::GetCombatTypeText() const
{
	return CombatType == EEnemyCombatType::Ranged ? FText::FromString(TEXT("원거리")) : FText::FromString(TEXT("근접"));
}

float AEnemyCharacter::GetAttackRecoveryTime() const
{
	const float LastHitTime = ActivePattern.ImpactDelay
		+ (FMath::Max(ActivePattern.HitCount, 1) - 1) * FMath::Max(ActivePattern.HitInterval, 0.01f);
	float AnimationLength = ActivePattern.Animation ? ActivePattern.Animation->GetPlayLength() : 0.0f;
	if (ActivePattern.AnimationDuration > 0.0f) AnimationLength = FMath::Min(AnimationLength, ActivePattern.AnimationDuration);
	float Cooldown = FMath::Max(ActivePattern.Cooldown, 0.0f) * (bEnraged ? EnrageCooldownMultiplier : 1.0f);
	Cooldown *= FMath::FRandRange(1.0f - AttackCooldownVariance, 1.0f + AttackCooldownVariance);
	return FMath::Max(AnimationLength, LastHitTime) + Cooldown;
}

bool AEnemyCharacter::ApplyStrikeHit(AActor* Target, const FEnemyAttackPattern& Pattern, float HalfAngleDegrees)
{
	if (!IsValid(Target)) return false;
	if (Pattern.AreaRadius > 0.0f)
	{
		if (GetTargetGap(Target) > Pattern.AreaRadius) return false;
	}
	else
	{
		// 타격 시점에 다시 검사해서 선딜 동안 거리를 벌리거나 옆으로 돌면 피할 수 있게 함
		if (!IsTargetInAttackRange(Target)) return false;
		const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero() && HalfAngleDegrees < 180.0f)
		{
			const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
			if (FVector::DotProduct(Forward, ToTarget) < FMath::Cos(FMath::DegreesToRadians(HalfAngleDegrees))) return false;
		}
	}

	UGameplayStatics::ApplyDamage(Target, Pattern.Damage, GetController(), this, UDamageType::StaticClass());
	if ((Pattern.KnockbackStrength > 0.0f || Pattern.KnockbackLift > 0.0f) && IsValid(Target))
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
		{
			const FVector Knockback = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * Pattern.KnockbackStrength
				+ FVector(0.0f, 0.0f, Pattern.KnockbackLift);
			TargetCharacter->LaunchCharacter(Knockback, true, true);
			UE_LOG(LogTemp, Display, TEXT("ENEMY_KNOCKBACK Enemy=%s Target=%s Pattern=%s"),
				*GetName(), *GetNameSafe(Target), *Pattern.Name.ToString());
		}
	}
	return true;
}

bool AEnemyCharacter::ExecuteCombatAttack(AActor* Target, const FEnemyAttackPattern& Pattern, int32 HitIndex)
{
	return ApplyStrikeHit(Target, Pattern, 180.0f);
}

void AEnemyCharacter::ResolveAttackImpact(TWeakObjectPtr<AActor> WeakTarget, int32 HitIndex)
{
	if (bIsDead) return;
	AActor* Target = WeakTarget.Get();
	const bool bHit = IsValid(Target) && ExecuteCombatAttack(Target, ActivePattern, HitIndex);
	UE_LOG(LogTemp, Display, TEXT("ENEMY_ATTACK_IMPACT Actor=%s Pattern=%s Hit=%d/%d Target=%s Result=%s"),
		*GetName(), *ActiveAttackName, HitIndex + 1, ActivePattern.HitCount, *GetNameSafe(Target), bHit ? TEXT("Hit") : TEXT("Miss"));

	// 범위 공격은 빗나가도 폭발이 보여야 하고, 단일 타격은 맞았을 때만 대상 위치에 보여 줌
	if (ActivePattern.ImpactEffect && (ActivePattern.AreaRadius > 0.0f || bHit))
	{
		const FVector EffectLocation = ActivePattern.AreaRadius > 0.0f || !IsValid(Target)
			? GetActorLocation() : Target->GetActorLocation();
		EnemyEffects::SpawnAtLocation(this, ActivePattern.ImpactEffect, EffectLocation, GetActorRotation(),
			FVector(ActivePattern.ImpactEffectScale), ActivePattern.ImpactDisabledEmitters);
	}

	// 같은 타이머 핸들을 재사용해서 연속 타격이 끝날 때까지 IsAttackInProgress가 유지되게 함
	if (HitIndex + 1 < ActivePattern.HitCount)
	{
		GetWorldTimerManager().SetTimer(AttackImpactTimer,
			FTimerDelegate::CreateUObject(this, &AEnemyCharacter::ResolveAttackImpact, WeakTarget, HitIndex + 1),
			FMath::Max(ActivePattern.HitInterval, 0.01f), false);
		return;
	}

	if (ActivePattern.bConsumesSelf)
	{
		if (bShowAttackDebug && ActivePattern.AreaRadius > 0.0f)
		{
			DrawDebugSphere(GetWorld(), GetActorLocation(), ActivePattern.AreaRadius, 16, FColor::Orange, false, 0.6f);
		}
		UE_LOG(LogTemp, Display, TEXT("ENEMY_SELF_DESTRUCT Actor=%s Hit=%s"), *GetName(), bHit ? TEXT("true") : TEXT("false"));
		HandleDeath(this);
		// 몸이 폭발로 사라진 것으로 보여 쓰러지는 모션 대신 바로 숨김. 폭발은 패턴의 ImpactEffect가 맡음
		SetActorHiddenInGame(true);
		SetLifeSpan(0.2f);
	}
}

void AEnemyCharacter::PerformLunge(TWeakObjectPtr<AActor> WeakTarget)
{
	if (bIsDead) return;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float Gravity = FMath::Abs(Movement->GetGravityZ());
	const float AirTime = ActivePattern.LungeLift > 0.0f && Gravity > KINDA_SMALL_NUMBER
		? 2.0f * ActivePattern.LungeLift / Gravity : 0.25f;
	float HorizontalSpeed = ActivePattern.LungeSpeed;

	// 발동 순간 대상을 다시 조준하고, 대상 바로 앞에 떨어지도록 속도를 줄임
	if (AActor* Target = WeakTarget.Get())
	{
		const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
		if (ToTarget.SizeSquared2D() > KINDA_SMALL_NUMBER) SetActorRotation(FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));
		const float Gap = GetTargetGap(Target);
		if (Gap < TNumericLimits<float>::Max()) HorizontalSpeed = FMath::Min(HorizontalSpeed, Gap / FMath::Max(AirTime, 0.05f));
	}

	FVector Launch = GetActorForwardVector().GetSafeNormal2D() * HorizontalSpeed;
	Launch.Z = ActivePattern.LungeLift;
	LaunchCharacter(Launch, true, true);
	UE_LOG(LogTemp, Display, TEXT("ENEMY_LUNGE Actor=%s Pattern=%s Speed=%.0f Lift=%.0f"),
		*GetName(), *ActiveAttackName, HorizontalSpeed, ActivePattern.LungeLift);
}

void AEnemyCharacter::CancelActiveAttack()
{
	GetWorldTimerManager().ClearTimer(AttackImpactTimer);
	GetWorldTimerManager().ClearTimer(LungeTimer);
	bPlayingActionAnimation = false;
}

void AEnemyCharacter::Stagger(const AActor* Source)
{
	if (bIsDead) return;
	const bool bInterrupted = IsAttackInProgress();
	CancelActiveAttack();
	AccumulatedPoiseDamage = 0.0f;
	StaggerEndTime = GetWorld()->GetTimeSeconds() + StaggerDuration;

	AEnemyAIController* AI = Cast<AEnemyAIController>(GetController());
	if (AI) AI->StopMovement();
	PlayActionAnimation(SelectDirectionalAnimation(Source, HitReactAnimation, HitReactBackAnimation, nullptr, nullptr), StaggerDuration);
	SetAIState(EEnemyAIState::Stagger);
	ShowDebugText(TEXT("경직"), FColor::Orange);
	UE_LOG(LogTemp, Display, TEXT("ENEMY_STAGGER Actor=%s Duration=%.2f InterruptedAttack=%s"),
		*GetName(), StaggerDuration, bInterrupted ? *ActiveAttackName : TEXT("None"));

	// 진행 중인 추격/공격/거리 조절 태스크를 끊고 경직 분기부터 다시 고르게 함
	if (AI) AI->RestartBehavior();
}

void AEnemyCharacter::Enrage()
{
	bEnraged = true;
	GetCharacterMovement()->MaxWalkSpeed = GetMovementSpeedForState(CurrentState);
	ShowDebugText(TEXT("격노"), FColor::Magenta);
	UE_LOG(LogTemp, Display, TEXT("ENEMY_ENRAGE Actor=%s Health=%.1f/%.1f"), *GetName(), CurrentHealth, MaxHealth);
}

void AEnemyCharacter::ShowDebugText(const FString& Text, const FColor& Color)
{
	if (!bShowAttackDebug) return;
	const FVector Offset(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 30.0f);
	DrawDebugString(GetWorld(), Offset, Text, this, Color, 1.0f, true, 1.2f);
}

bool AEnemyCharacter::PerformAttack(AActor* Target)
{
	if (!CanStartAttack(Target)) return false;
	const int32 PatternIndex = ChooseAttackPattern(Target);
	if (PatternIndex == INDEX_NONE) return false;

	const double Now = GetWorld()->GetTimeSeconds();
	ActivePattern = AttackPatterns[PatternIndex];
	if (!ActivePattern.Animation) ActivePattern.Animation = AttackAnimation;
	ActiveAttackName = ActivePattern.Name.ToString();
	LastPatternIndex = PatternIndex;
	if (PatternReadyTimes.Num() != AttackPatterns.Num()) PatternReadyTimes.SetNumZeroed(AttackPatterns.Num());
	PatternReadyTimes[PatternIndex] = Now + ActivePattern.PatternCooldown;

	const FVector Direction = Target->GetActorLocation() - GetActorLocation();
	if (AEnemyAIController* AI = Cast<AEnemyAIController>(GetController())) AI->StopMovement();
	SetActorRotation(FRotator(0.0f, Direction.Rotation().Yaw, 0.0f));
	NextAttackTime = Now + GetAttackRecoveryTime();
	if (UAnimSequence* Animation = ActivePattern.Animation)
	{
		PlayActionAnimation(Animation, ActivePattern.AnimationDuration);
		UE_LOG(LogTemp, Display, TEXT("ENEMY_ANIMATION Attack=%s Duration=%.2f"), *Animation->GetName(),
			ActivePattern.AnimationDuration > 0.0f ? FMath::Min(ActivePattern.AnimationDuration, Animation->GetPlayLength()) : Animation->GetPlayLength());
	}
	BP_OnAttack(Target);

	const TWeakObjectPtr<AActor> WeakTarget(Target);
	const bool bLunges = ActivePattern.LungeSpeed > 0.0f || ActivePattern.LungeLift > 0.0f;
	if (bLunges)
	{
		if (ActivePattern.LungeDelay > 0.0f)
		{
			GetWorldTimerManager().SetTimer(LungeTimer,
				FTimerDelegate::CreateUObject(this, &AEnemyCharacter::PerformLunge, WeakTarget), ActivePattern.LungeDelay, false);
		}
		else
		{
			PerformLunge(WeakTarget);
		}
	}
	if (ActivePattern.ImpactDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(AttackImpactTimer,
			FTimerDelegate::CreateUObject(this, &AEnemyCharacter::ResolveAttackImpact, WeakTarget, 0), ActivePattern.ImpactDelay, false);
	}
	else
	{
		ResolveAttackImpact(WeakTarget, 0);
	}

	// 범위 공격은 떨어질 자리를 미리 보여 줘서 피할 수 있게 함
	if (bShowAttackDebug && ActivePattern.AreaRadius > 0.0f)
	{
		const FVector Center = (bLunges ? Target->GetActorLocation() : GetActorLocation())
			- FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 5.0f);
		DrawDebugCircle(GetWorld(), Center, ActivePattern.AreaRadius, 32, FColor::Orange, false,
			FMath::Max(ActivePattern.ImpactDelay, 0.1f), 0, 3.0f, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), false);
	}

	UE_LOG(LogTemp, Display, TEXT("ENEMY_ATTACK_PATTERN Actor=%s Pattern=%s Damage=%.1f Cooldown=%.2f ImpactDelay=%.2f Hits=%d Enraged=%s"),
		*GetName(), *ActiveAttackName, ActivePattern.Damage, ActivePattern.Cooldown, ActivePattern.ImpactDelay,
		ActivePattern.HitCount, bEnraged ? TEXT("true") : TEXT("false"));
	++SuccessfulAttackCount;
	ShowDebugText(ActiveAttackName, FColor::Red);
	UE_LOG(LogTemp, Display, TEXT("%s 공격 Enemy=%s Target=%s"), *GetCombatTypeText().ToString(), *GetName(), *GetNameSafe(Target));
	UE_LOG(LogTemp, Display, TEXT("ENEMY_BT_TEST AttackSucceeded Actor=%s Count=%d Target=%s"),
		*GetName(), SuccessfulAttackCount, *GetNameSafe(Target));
	return true;
}

void AEnemyCharacter::SetAIState(EEnemyAIState NewState)
{
	if (bIsDead) return;
	if (CurrentState == NewState)
	{
		return;
	}

	const FString PreviousState = StaticEnum<EEnemyAIState>()->GetNameStringByValue(static_cast<int64>(CurrentState));
	const FString NextState = StaticEnum<EEnemyAIState>()->GetNameStringByValue(static_cast<int64>(NewState));
	CurrentState = NewState;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = GetMovementSpeedForState(NewState);
	// 거리 조절 중에는 이동 방향이 아니라 대상을 바라보며 옆걸음/뒷걸음함
	Movement->bOrientRotationToMovement = NewState != EEnemyAIState::Reposition && NewState != EEnemyAIState::Retreat;
	UE_LOG(LogTemp, Display, TEXT("ENEMY_BT_TEST StateChanged Actor=%s From=%s To=%s"),
		*GetName(), *PreviousState, *NextState);
}
