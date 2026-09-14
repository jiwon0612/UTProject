#include "PYW/EnemyCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "PYW/EnemyAIController.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UBlendSpace> MovementAsset(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AttackAsset(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"));
	LocomotionAnimation = MovementAsset.Object;
	AttackAnimation = AttackAsset.Object;
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
	PlayLocomotionAnimation();
}

void AEnemyCharacter::PlayLocomotionAnimation()
{
	if (LocomotionAnimation) GetMesh()->PlayAnimation(LocomotionAnimation, true);
	bPlayingAttackAnimation = false;
}

float AEnemyCharacter::GetAttackDuration() const
{
	return FMath::Max(FMath::Max(AttackCooldown, 0.05f), AttackAnimation ? AttackAnimation->GetPlayLength() : 0.0f);
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
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

bool AEnemyCharacter::IsTargetInAttackRange(const AActor* Target) const
{
	return IsValid(Target) && Target != this
		&& FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(AttackRange);
}

bool AEnemyCharacter::PerformAttack(AActor* Target)
{
	if (!IsTargetInAttackRange(Target) || GetWorld()->GetTimeSeconds() < NextAttackTime)
	{
		return false;
	}

	const FVector Direction = Target->GetActorLocation() - GetActorLocation();
	if (AEnemyAIController* AI = Cast<AEnemyAIController>(GetController())) AI->StopMovement();
	SetActorRotation(FRotator(0.0f, Direction.Rotation().Yaw, 0.0f));
	NextAttackTime = GetWorld()->GetTimeSeconds() + GetAttackDuration();
	if (AttackAnimation)
	{
		GetMesh()->PlayAnimation(AttackAnimation, false);
		bPlayingAttackAnimation = true;
		AttackAnimationEndTime = GetWorld()->GetTimeSeconds() + AttackAnimation->GetPlayLength();
		UE_LOG(LogTemp, Display, TEXT("ENEMY_ANIMATION Attack=%s Duration=%.2f"), *AttackAnimation->GetName(), AttackAnimation->GetPlayLength());
	}
	BP_OnAttack(Target);
	UGameplayStatics::ApplyDamage(Target, AttackDamage, GetController(), this, UDamageType::StaticClass());
	++SuccessfulAttackCount;
	if (bShowAttackDebug)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 1.0f, FColor::Red, TEXT("공격"));
		UE_LOG(LogTemp, Display, TEXT("공격 Enemy=%s Target=%s"), *GetName(), *GetNameSafe(Target));
	}
	UE_LOG(LogTemp, Display, TEXT("ENEMY_BT_TEST AttackSucceeded Actor=%s Count=%d Target=%s"),
		*GetName(), SuccessfulAttackCount, *GetNameSafe(Target));
	return true;
}

void AEnemyCharacter::SetAIState(EEnemyAIState NewState)
{
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
