#include "PYW/EnemyCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PYW/EnemyAIController.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

bool AEnemyCharacter::PerformAttack(AActor* Target)
{
	if (!IsValid(Target) || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(AttackRange))
	{
		return false;
	}

	const FVector Direction = Target->GetActorLocation() - GetActorLocation();
	SetActorRotation(Direction.Rotation());
	BP_OnAttack(Target);
	UGameplayStatics::ApplyDamage(Target, AttackDamage, GetController(), this, UDamageType::StaticClass());
	++SuccessfulAttackCount;
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
	UE_LOG(LogTemp, Display, TEXT("ENEMY_BT_TEST StateChanged Actor=%s From=%s To=%s"),
		*GetName(), *PreviousState, *NextState);
}
