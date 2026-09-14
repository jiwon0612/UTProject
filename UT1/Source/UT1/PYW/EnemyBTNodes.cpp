#include "PYW/EnemyBTNodes.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "PYW/EnemyAIController.h"
#include "PYW/EnemyCharacter.h"

namespace
{
	struct FTimedTaskMemory
	{
		float RemainingTime = 0.0f;
	};

	AEnemyCharacter* GetEnemy(const UBehaviorTreeComponent& OwnerComp)
	{
		const AAIController* Controller = OwnerComp.GetAIOwner();
		return Controller ? Cast<AEnemyCharacter>(Controller->GetPawn()) : nullptr;
	}

	AActor* GetTarget(const UBehaviorTreeComponent& OwnerComp)
	{
		const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
		return Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(AEnemyAIController::TargetActorKey)) : nullptr;
	}

	bool IsWithinAttackRange(const AEnemyCharacter* Enemy, const AActor* Target)
	{
		return IsValid(Enemy) && Enemy->IsTargetInAttackRange(Target);
	}
}

UEnemyBTService_FindTarget::UEnemyBTService_FindTarget()
{
	NodeName = TEXT("Find Player Target");
	Interval = 0.2f;
	RandomDeviation = 0.02f;
	bCallTickOnSearchStart = true;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UEnemyBTService_FindTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UpdateTarget(OwnerComp);
}

void UEnemyBTService_FindTarget::OnSearchStart(FBehaviorTreeSearchData& SearchData)
{
	Super::OnSearchStart(SearchData);
	UpdateTarget(SearchData.OwnerComp);
}

void UEnemyBTService_FindTarget::UpdateTarget(UBehaviorTreeComponent& OwnerComp) const
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Controller || !Enemy || !Blackboard)
	{
		return;
	}

	AActor* CurrentTarget = GetTarget(OwnerComp);
	if (IsValid(CurrentTarget))
	{
		const float DistanceSquared = FVector::DistSquared2D(Enemy->GetActorLocation(), CurrentTarget->GetActorLocation());
		if (DistanceSquared <= FMath::Square(Enemy->LoseTargetRange))
		{
			return;
		}
		Blackboard->ClearValue(AEnemyAIController::TargetActorKey);
	}

	APlayerController* PlayerController = Enemy->GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!IsValid(PlayerPawn))
	{
		return;
	}

	const float DistanceSquared = FVector::DistSquared2D(Enemy->GetActorLocation(), PlayerPawn->GetActorLocation());
	if (DistanceSquared <= FMath::Square(Enemy->DetectionRange) && Controller->LineOfSightTo(PlayerPawn))
	{
		Blackboard->SetValueAsObject(AEnemyAIController::TargetActorKey, PlayerPawn);
	}
}

UEnemyBTDecorator_HasTarget::UEnemyBTDecorator_HasTarget()
{
	NodeName = TEXT("Has Target");
}

bool UEnemyBTDecorator_HasTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	return IsValid(GetTarget(OwnerComp));
}

UEnemyBTDecorator_CanAttack::UEnemyBTDecorator_CanAttack()
{
	NodeName = TEXT("Target In Attack Range");
}

bool UEnemyBTDecorator_CanAttack::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	return IsWithinAttackRange(GetEnemy(OwnerComp), GetTarget(OwnerComp));
}

UEnemyBTTask_FindPatrolPoint::UEnemyBTTask_FindPatrolPoint()
{
	NodeName = TEXT("Choose Patrol Point");
}

EBTNodeResult::Type UEnemyBTTask_FindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	UNavigationSystemV1* Navigation = Enemy ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Enemy->GetWorld()) : nullptr;
	if (!Enemy || !Blackboard || !Navigation)
	{
		return EBTNodeResult::Failed;
	}

	FNavLocation PatrolPoint;
	if (IsValid(GetTarget(OwnerComp))) return EBTNodeResult::Failed;
	if (!Navigation->GetRandomReachablePointInRadius(Enemy->GetActorLocation(), Enemy->PatrolRadius, PatrolPoint))
	{
		return EBTNodeResult::Failed;
	}

	Blackboard->SetValueAsVector(AEnemyAIController::PatrolLocationKey, PatrolPoint.Location);
	return EBTNodeResult::Succeeded;
}

UEnemyBTTask_WalkPatrol::UEnemyBTTask_WalkPatrol()
{
	NodeName = TEXT("Walk");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UEnemyBTTask_WalkPatrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Controller || !Enemy || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	Enemy->SetAIState(EEnemyAIState::Walk);
	const FVector Destination = Blackboard->GetValueAsVector(AEnemyAIController::PatrolLocationKey);
	const EPathFollowingRequestResult::Type Result = Controller->MoveToLocation(Destination, 45.0f, true);
	return Result == EPathFollowingRequestResult::Failed ? EBTNodeResult::Failed
		: (Result == EPathFollowingRequestResult::AlreadyAtGoal ? EBTNodeResult::Succeeded : EBTNodeResult::InProgress);
}

EBTNodeResult::Type UEnemyBTTask_WalkPatrol::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* Controller = OwnerComp.GetAIOwner())
	{
		Controller->StopMovement();
	}
	return EBTNodeResult::Aborted;
}

void UEnemyBTTask_WalkPatrol::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Controller || !Enemy || !Blackboard || IsValid(GetTarget(OwnerComp)))
	{
		if (Controller) Controller->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FVector Destination = Blackboard->GetValueAsVector(AEnemyAIController::PatrolLocationKey);
	if (FVector::DistSquared2D(Enemy->GetActorLocation(), Destination) <= FMath::Square(55.0f))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
	else if (Controller->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

UEnemyBTTask_Chase::UEnemyBTTask_Chase()
{
	NodeName = TEXT("Chase");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UEnemyBTTask_Chase::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	AActor* Target = GetTarget(OwnerComp);
	if (!Controller || !Enemy || !IsValid(Target))
	{
		return EBTNodeResult::Failed;
	}

	if (IsWithinAttackRange(Enemy, Target))
	{
		return EBTNodeResult::Succeeded;
	}

	Enemy->SetAIState(EEnemyAIState::Chase);
	const EPathFollowingRequestResult::Type Result = Controller->MoveToActor(Target, 5.0f, false);
	return Result == EPathFollowingRequestResult::Failed ? EBTNodeResult::Failed : EBTNodeResult::InProgress;
}

EBTNodeResult::Type UEnemyBTTask_Chase::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* Controller = OwnerComp.GetAIOwner())
	{
		Controller->StopMovement();
	}
	return EBTNodeResult::Aborted;
}

void UEnemyBTTask_Chase::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	AActor* Target = GetTarget(OwnerComp);
	if (!Controller || !Enemy || !IsValid(Target))
	{
		if (Controller) Controller->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (IsWithinAttackRange(Enemy, Target))
	{
		Controller->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
	else if (Controller->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

UEnemyBTTask_Attack::UEnemyBTTask_Attack()
{
	NodeName = TEXT("Attack");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UEnemyBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	AActor* Target = GetTarget(OwnerComp);
	if (!Enemy || !Enemy->PerformAttack(Target))
	{
		return EBTNodeResult::Failed;
	}

	Enemy->SetAIState(EEnemyAIState::Attack);
	reinterpret_cast<FTimedTaskMemory*>(NodeMemory)->RemainingTime = Enemy->GetAttackDuration();
	return EBTNodeResult::InProgress;
}

void UEnemyBTTask_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FTimedTaskMemory* Memory = reinterpret_cast<FTimedTaskMemory*>(NodeMemory);
	Memory->RemainingTime -= DeltaSeconds;
	if (Memory->RemainingTime <= 0.0f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UEnemyBTTask_Attack::GetInstanceMemorySize() const
{
	return sizeof(FTimedTaskMemory);
}

UEnemyBTTask_Idle::UEnemyBTTask_Idle()
{
	NodeName = TEXT("Idle");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UEnemyBTTask_Idle::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AEnemyCharacter* Enemy = GetEnemy(OwnerComp))
	{
		Enemy->SetAIState(EEnemyAIState::Idle);
	}
	if (AAIController* Controller = OwnerComp.GetAIOwner())
	{
		Controller->StopMovement();
	}
	reinterpret_cast<FTimedTaskMemory*>(NodeMemory)->RemainingTime = FMath::FRandRange(1.0f, 2.5f);
	return EBTNodeResult::InProgress;
}

void UEnemyBTTask_Idle::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FTimedTaskMemory* Memory = reinterpret_cast<FTimedTaskMemory*>(NodeMemory);
	Memory->RemainingTime -= DeltaSeconds;
	if (IsValid(GetTarget(OwnerComp)) || Memory->RemainingTime <= 0.0f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UEnemyBTTask_Idle::GetInstanceMemorySize() const
{
	return sizeof(FTimedTaskMemory);
}

