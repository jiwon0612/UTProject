#include "PYW/Entities/EnemyCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "BrainComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "PYW/AI/EnemyAIController.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UBlendSpace> MovementAsset(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AttackAsset(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathAsset(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01"));
	LocomotionAnimation = MovementAsset.Object;
	AttackAnimation = AttackAsset.Object;
	DeathAnimation = DeathAsset.Object;

	FEnemyAttackPattern& DefaultPattern = AttackPatterns.AddDefaulted_GetRef();
	DefaultPattern.Name = TEXT("Default");
	DefaultPattern.Animation = AttackAsset.Object;
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
	CurrentHealth = MaxHealth;
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

float AEnemyCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (bDead || DamageAmount <= 0.0f) return 0.0f;
	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (AppliedDamage <= 0.0f) return 0.0f;

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - AppliedDamage);
	UE_LOG(LogTemp, Display, TEXT("ENEMY_DAMAGE Actor=%s Damage=%.1f Health=%.1f/%.1f"),
		*GetName(), AppliedDamage, CurrentHealth, MaxHealth);
	if (CurrentHealth <= 0.0f)
	{
		Die(EventInstigator, DamageCauser);
		return AppliedDamage;
	}

	// 감지 범위 밖이나 시야 밖에서 맞아도 공격자를 바로 추적함
	APawn* InstigatorPawn = EventInstigator ? EventInstigator->GetPawn() : nullptr;
	if (IsValid(InstigatorPawn) && !InstigatorPawn->IsA<AEnemyCharacter>())
	{
		if (AEnemyAIController* AI = Cast<AEnemyAIController>(GetController()))
		{
			AI->SetTargetActor(InstigatorPawn);
		}
	}
	return AppliedDamage;
}

void AEnemyCharacter::Die(AController* Killer, AActor* DamageCauser)
{
	if (bDead) return;
	bDead = true;
	CurrentHealth = 0.0f;
	bPlayingAttackAnimation = false;
	// 대기 중인 타격, 연타, 연사 타이머가 사망 후 발동하지 않도록 정리함
	GetWorldTimerManager().ClearAllTimersForObject(this);

	if (AEnemyAIController* AI = Cast<AEnemyAIController>(GetController()))
	{
		AI->StopMovement();
		if (UBrainComponent* Brain = AI->GetBrainComponent()) Brain->StopLogic(TEXT("Enemy died"));
	}
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	float DeathDuration = 0.0f;
	if (DeathAnimation)
	{
		GetMesh()->PlayAnimation(DeathAnimation, false);
		DeathDuration = DeathAnimation->GetPlayLength();
	}
	SetLifeSpan(FMath::Max(0.1f, DeathDuration + DeathCleanupDelay));
	UE_LOG(LogTemp, Display, TEXT("ENEMY_DEATH Actor=%s Animation=%s Duration=%.2f Causer=%s"),
		*GetName(), *GetNameSafe(DeathAnimation), DeathDuration, *GetNameSafe(DamageCauser));
}

void AEnemyCharacter::PlayLocomotionAnimation()
{
	if (LocomotionAnimation) GetMesh()->PlayAnimation(LocomotionAnimation, true);
	bPlayingAttackAnimation = false;
}

void AEnemyCharacter::SelectNextAttackPattern()
{
	if (AttackPatterns.IsEmpty())
	{
		ActivePattern = FEnemyAttackPattern();
	}
	else
	{
		const int32 PatternIndex = NextAttackPatternIndex % AttackPatterns.Num();
		NextAttackPatternIndex = (PatternIndex + 1) % AttackPatterns.Num();
		ActivePattern = AttackPatterns[PatternIndex];
	}
	if (!ActivePattern.Animation) ActivePattern.Animation = AttackAnimation;
	ActiveAttackName = ActivePattern.Name.ToString();
}

float AEnemyCharacter::GetAttackDuration() const
{
	// 다음 공격이 이전 공격의 남은 타격과 겹치지 않도록 마지막 타격 시점까지 포함함
	const float LastHitTime = ActivePattern.ImpactDelay
		+ (FMath::Max(ActivePattern.HitCount, 1) - 1) * FMath::Max(ActivePattern.HitInterval, 0.01f);
	const float AnimationLength = ActivePattern.Animation ? ActivePattern.Animation->GetPlayLength() : 0.0f;
	return FMath::Max3(FMath::Max(ActivePattern.Cooldown, 0.05f), AnimationLength, LastHitTime);
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDead) return;
	if (bPlayingAttackAnimation && GetWorld()->GetTimeSeconds() >= AttackAnimationEndTime)
	{
		PlayLocomotionAnimation();
	}
	if (!bPlayingAttackAnimation)
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
	if (bDead) UE_LOG(LogTemp, Display, TEXT("ENEMY_DEATH_REMOVED Actor=%s"), *GetName());
	Super::Destroyed();
}

bool AEnemyCharacter::IsTargetInAttackRange(const AActor* Target) const
{
	if (!IsValid(Target) || Target == this) return false;
	const ACharacter* TargetCharacter = Cast<ACharacter>(Target);
	const UCapsuleComponent* TargetCapsule = TargetCharacter ? TargetCharacter->GetCapsuleComponent() : nullptr;
	const float TargetRadius = TargetCapsule ? TargetCapsule->GetScaledCapsuleRadius() : 0.0f;
	const float TargetHalfHeight = TargetCapsule ? TargetCapsule->GetScaledCapsuleHalfHeight() : 0.0f;
	const float Reach = FMath::Max(AttackRange, 0.0f) + GetCapsuleComponent()->GetScaledCapsuleRadius() + TargetRadius;
	const FVector Offset = Target->GetActorLocation() - GetActorLocation();
	return Offset.SizeSquared2D() <= FMath::Square(Reach)
		&& FMath::Abs(Offset.Z) <= GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + TargetHalfHeight;
}

bool AEnemyCharacter::IsAttackInProgress() const
{
	return bPlayingAttackAnimation || GetWorldTimerManager().IsTimerActive(AttackImpactTimer);
}

void AEnemyCharacter::FaceTarget(const AActor* Target, float DeltaSeconds)
{
	if (bDead || !IsValid(Target) || IsAttackInProgress()) return;
	const FVector Offset = Target->GetActorLocation() - GetActorLocation();
	if (Offset.SizeSquared2D() <= KINDA_SMALL_NUMBER) return;
	const FRotator Desired(0.0f, Offset.Rotation().Yaw, 0.0f);
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Desired, DeltaSeconds,
		static_cast<float>(GetCharacterMovement()->RotationRate.Yaw)));
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

bool AEnemyCharacter::ExecuteCombatAttack(AActor* Target, const FEnemyAttackPattern& Pattern, int32 HitIndex)
{
	// 타격 시점에 다시 검사해서 선딜 동안 거리를 벌리면 회피할 수 있게 함
	if (!IsTargetInAttackRange(Target)) return false;
	UGameplayStatics::ApplyDamage(Target, Pattern.Damage, GetController(), this, UDamageType::StaticClass());
	return true;
}

void AEnemyCharacter::ResolveAttackImpact(TWeakObjectPtr<AActor> WeakTarget, int32 HitIndex)
{
	AActor* Target = WeakTarget.Get();
	if (bDead || !IsValid(Target)) return;
	const bool bHit = ExecuteCombatAttack(Target, ActivePattern, HitIndex);
	UE_LOG(LogTemp, Display, TEXT("ENEMY_ATTACK_IMPACT Actor=%s Pattern=%s Hit=%d/%d Target=%s Result=%s"),
		*GetName(), *ActiveAttackName, HitIndex + 1, ActivePattern.HitCount, *GetNameSafe(Target), bHit ? TEXT("Hit") : TEXT("Miss"));

	// 같은 타이머 핸들을 재사용해서 연속 타격이 끝날 때까지 IsAttackInProgress가 유지되게 함
	if (HitIndex + 1 < ActivePattern.HitCount)
	{
		GetWorldTimerManager().SetTimer(AttackImpactTimer,
			FTimerDelegate::CreateUObject(this, &AEnemyCharacter::ResolveAttackImpact, WeakTarget, HitIndex + 1),
			FMath::Max(ActivePattern.HitInterval, 0.01f), false);
	}
}

bool AEnemyCharacter::PerformAttack(AActor* Target)
{
	if (bDead || !IsTargetInAttackRange(Target) || GetWorld()->GetTimeSeconds() < NextAttackTime)
	{
		return false;
	}

	const FVector Direction = Target->GetActorLocation() - GetActorLocation();
	if (AEnemyAIController* AI = Cast<AEnemyAIController>(GetController())) AI->StopMovement();
	SetActorRotation(FRotator(0.0f, Direction.Rotation().Yaw, 0.0f));
	SelectNextAttackPattern();
	NextAttackTime = GetWorld()->GetTimeSeconds() + GetAttackDuration();
	if (UAnimSequence* Animation = ActivePattern.Animation)
	{
		GetMesh()->PlayAnimation(Animation, false);
		bPlayingAttackAnimation = true;
		AttackAnimationEndTime = GetWorld()->GetTimeSeconds() + Animation->GetPlayLength();
		UE_LOG(LogTemp, Display, TEXT("ENEMY_ANIMATION Attack=%s Duration=%.2f"), *Animation->GetName(), Animation->GetPlayLength());
	}
	BP_OnAttack(Target);
	const TWeakObjectPtr<AActor> WeakTarget(Target);
	if (ActivePattern.ImpactDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(AttackImpactTimer,
			FTimerDelegate::CreateUObject(this, &AEnemyCharacter::ResolveAttackImpact, WeakTarget, 0), ActivePattern.ImpactDelay, false);
	}
	else
	{
		ResolveAttackImpact(WeakTarget, 0);
	}
	UE_LOG(LogTemp, Display, TEXT("ENEMY_ATTACK_PATTERN Actor=%s Pattern=%s Damage=%.1f Cooldown=%.2f ImpactDelay=%.2f Hits=%d"),
		*GetName(), *ActiveAttackName, ActivePattern.Damage, ActivePattern.Cooldown, ActivePattern.ImpactDelay, ActivePattern.HitCount);
	++SuccessfulAttackCount;
	if (bShowAttackDebug)
	{
		const FString DebugText = FString::Printf(TEXT("%s 공격"), *GetCombatTypeText().ToString());
		if (GEngine) GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 1.0f, FColor::Red, DebugText);
		UE_LOG(LogTemp, Display, TEXT("%s Enemy=%s Target=%s"), *DebugText, *GetName(), *GetNameSafe(Target));
	}
	UE_LOG(LogTemp, Display, TEXT("ENEMY_BT_TEST AttackSucceeded Actor=%s Count=%d Target=%s"),
		*GetName(), SuccessfulAttackCount, *GetNameSafe(Target));
	return true;
}

void AEnemyCharacter::SetAIState(EEnemyAIState NewState)
{
	if (bDead) return;
	if (CurrentState == NewState)
	{
		return;
	}

	const FString PreviousState = StaticEnum<EEnemyAIState>()->GetNameStringByValue(static_cast<int64>(CurrentState));
	const FString NextState = StaticEnum<EEnemyAIState>()->GetNameStringByValue(static_cast<int64>(NewState));
	CurrentState = NewState;
	GetCharacterMovement()->MaxWalkSpeed = NewState == EEnemyAIState::Chase ? ChaseSpeed : WalkSpeed;
	UE_LOG(LogTemp, Display, TEXT("ENEMY_BT_TEST StateChanged Actor=%s From=%s To=%s"),
		*GetName(), *PreviousState, *NextState);
}
