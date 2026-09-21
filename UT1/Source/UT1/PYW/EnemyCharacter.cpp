#include "PYW/EnemyCharacter.h"

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
#include "PYW/EnemyAIController.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UBlendSpace> MovementAsset(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AttackAsset(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathAsset(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01"));
	LocomotionAnimation = MovementAsset.Object;
	AttackAnimation = AttackAsset.Object;
	if (AttackAsset.Succeeded()) AttackAnimations.Add(AttackAsset.Object);
	DeathAnimation = DeathAsset.Object;
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

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
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
	if (CurrentHealth <= 0.0f) Die(EventInstigator, DamageCauser);
	return AppliedDamage;
}

void AEnemyCharacter::Die(AController* Killer, AActor* DamageCauser)
{
	if (bDead) return;
	bDead = true;
	CurrentHealth = 0.0f;
	bPlayingAttackAnimation = false;

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

UAnimSequence* AEnemyCharacter::SelectNextAttackAnimation()
{
	if (AttackAnimations.IsEmpty()) return AttackAnimation;
	const int32 AnimationIndex = NextAttackAnimationIndex % AttackAnimations.Num();
	NextAttackAnimationIndex = (AnimationIndex + 1) % AttackAnimations.Num();
	return AttackAnimations[AnimationIndex];
}

float AEnemyCharacter::GetAttackDuration() const
{
	return FMath::Max(FMath::Max(AttackCooldown, 0.05f), AttackAnimation ? AttackAnimation->GetPlayLength() : 0.0f);
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

bool AEnemyCharacter::ExecuteCombatAttack(AActor* Target)
{
	UGameplayStatics::ApplyDamage(Target, AttackDamage, GetController(), this, UDamageType::StaticClass());
	return true;
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
	AttackAnimation = SelectNextAttackAnimation();
	NextAttackTime = GetWorld()->GetTimeSeconds() + GetAttackDuration();
	if (AttackAnimation)
	{
		GetMesh()->PlayAnimation(AttackAnimation, false);
		bPlayingAttackAnimation = true;
		AttackAnimationEndTime = GetWorld()->GetTimeSeconds() + AttackAnimation->GetPlayLength();
		UE_LOG(LogTemp, Display, TEXT("ENEMY_ANIMATION Attack=%s Duration=%.2f"), *AttackAnimation->GetName(), AttackAnimation->GetPlayLength());
	}
	BP_OnAttack(Target);
	if (!ExecuteCombatAttack(Target)) return false;
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
