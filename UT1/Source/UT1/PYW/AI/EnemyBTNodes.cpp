#include "PYW/AI/EnemyBTNodes.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "PYW/AI/EnemyAIController.h"
#include "PYW/Entities/EnemyCharacter.h"

namespace
{
	struct FTimedTaskMemory
	{
		float RemainingTime = 0.0f;
	};

	struct FRepositionMemory
	{
		float RepathTime = 0.0f;
		float StrafeSign = 1.0f;
	};

	struct FSearchMemory
	{
		float LookTimeRemaining = 0.0f;
		float NextTurnTime = 0.0f;
		float LookYaw = 0.0f;
		bool bLooking = false;
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

	// 추격을 멈춰도 되는 시점임. 바로 공격할 수 있거나, 쿨다운 중이지만 거리 조절로 넘어갈 만큼 가까울 때임
	bool IsChaseComplete(const AEnemyCharacter* Enemy, const AActor* Target)
	{
		return Enemy->CanStartAttack(Target)
			|| (Enemy->GetAttackCooldownRemaining() > 0.0f && Enemy->IsInCombatBand(Target));
	}

	EPathFollowingRequestResult::Type MoveToNavigable(AAIController* Controller, const FVector& Destination)
	{
		FVector Goal = Destination;
		if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Controller->GetWorld()))
		{
			FNavLocation Projected;
			if (Navigation->ProjectPointToNavigation(Destination, Projected, FVector(200.0f, 200.0f, 300.0f)))
			{
				Goal = Projected.Location;
			}
		}
		return Controller->MoveToLocation(Goal, 30.0f, false, true, false, true);
	}

	void UpdateRepositionMove(AAIController* Controller, AEnemyCharacter* Enemy, AActor* Target, FRepositionMemory* Memory)
	{
		const FVector EnemyLocation = Enemy->GetActorLocation();
		const FVector TargetLocation = Target->GetActorLocation();
		const FVector FromTarget = (EnemyLocation - TargetLocation).GetSafeNormal2D();
		const float Gap = Enemy->GetTargetGap(Target);

		if (Enemy->CombatMovement == EEnemyCombatMovement::Kite && Gap < Enemy->RetreatDistance)
		{
			Enemy->SetAIState(EEnemyAIState::Retreat);
			MoveToNavigable(Controller, EnemyLocation + FromTarget * (Enemy->RetreatDistance - Gap + 200.0f));
			Memory->RepathTime = 0.6f;
			return;
		}

		Enemy->SetAIState(EEnemyAIState::Reposition);
		if (Enemy->CombatMovement == EEnemyCombatMovement::HoldGround)
		{
			if (Gap > Enemy->AttackRange * 0.9f) Controller->MoveToActor(Target, Enemy->GetChaseAcceptanceRadius(), false);
			else Controller->StopMovement();
			Memory->RepathTime = 0.5f;
			return;
		}

		// 교전 거리를 유지한 채 대상 주위 원을 따라 옆으로 이동함. 가끔 방향을 바꿔 움직임을 읽기 어렵게 함
		if (FMath::FRand() < 0.3f) Memory->StrafeSign *= -1.0f;
		const float Distance = FVector::Dist2D(EnemyLocation, TargetLocation);
		const float RadiusSum = Gap < TNumericLimits<float>::Max() ? FMath::Max(Distance - Gap, 0.0f) : 0.0f;
		const float MinOrbit = RadiusSum + Enemy->AttackRange * 0.8f;
		const float MaxOrbit = RadiusSum + Enemy->AttackRange + Enemy->CombatBandPadding * 0.5f;
		const float Orbit = FMath::Clamp(Distance, MinOrbit, MaxOrbit);
		const FVector Offset = FromTarget.RotateAngleAxis(Memory->StrafeSign * FMath::FRandRange(35.0f, 65.0f), FVector::UpVector) * Orbit;
		MoveToNavigable(Controller, TargetLocation + Offset);
		Memory->RepathTime = FMath::FRandRange(0.9f, 1.6f);
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
	if (IsValid(CurrentTarget) && !AEnemyCharacter::IsValidCombatTarget(CurrentTarget))
	{
		// 죽은 대상은 수색할 이유가 없어서 마지막 위치까지 지우고 순찰로 돌아감
		Blackboard->ClearValue(AEnemyAIController::TargetActorKey);
		Blackboard->ClearValue(AEnemyAIController::LastKnownLocationKey);
		Controller->StopMovement();
		UE_LOG(LogTemp, Display, TEXT("ENEMY_TARGET_LOST Enemy=%s Target=%s Reason=Dead"),
			*Enemy->GetName(), *GetNameSafe(CurrentTarget));
		return;
	}
	if (IsValid(CurrentTarget))
	{
		if (Controller->LineOfSightTo(CurrentTarget))
		{
			Enemy->MarkTargetSeen();
			Blackboard->SetValueAsVector(AEnemyAIController::LastKnownLocationKey, CurrentTarget->GetActorLocation());
		}
		const float DistanceSquared = FVector::DistSquared2D(Enemy->GetActorLocation(), CurrentTarget->GetActorLocation());
		const bool bTooFar = DistanceSquared > FMath::Square(Enemy->LoseTargetRange);
		const bool bOutOfSight = Enemy->GetTimeSinceTargetSeen() > Enemy->LoseSightTime;
		if (!bTooFar && !bOutOfSight)
		{
			return;
		}
		// 마지막으로 본 위치는 남겨 둬서 수색 분기가 이어받게 함
		Blackboard->ClearValue(AEnemyAIController::TargetActorKey);
		UE_LOG(LogTemp, Display, TEXT("ENEMY_TARGET_LOST Enemy=%s Target=%s Reason=%s"),
			*Enemy->GetName(), *GetNameSafe(CurrentTarget), bTooFar ? TEXT("TooFar") : TEXT("OutOfSight"));
		return;
	}
	if (Blackboard->GetValueAsObject(AEnemyAIController::TargetActorKey))
	{
		Blackboard->ClearValue(AEnemyAIController::TargetActorKey);
	}

	APlayerController* PlayerController = Enemy->GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!AEnemyCharacter::IsValidCombatTarget(PlayerPawn))
	{
		return;
	}

	const float DistanceSquared = FVector::DistSquared2D(Enemy->GetActorLocation(), PlayerPawn->GetActorLocation());
	if (DistanceSquared <= FMath::Square(Enemy->DetectionRange) && Controller->LineOfSightTo(PlayerPawn))
	{
		Blackboard->SetValueAsObject(AEnemyAIController::TargetActorKey, PlayerPawn);
		Blackboard->SetValueAsVector(AEnemyAIController::LastKnownLocationKey, PlayerPawn->GetActorLocation());
		Enemy->MarkTargetSeen();
		Enemy->BeginAlert();
		UE_LOG(LogTemp, Display, TEXT("ENEMY_TARGET_ACQUIRED Enemy=%s Type=%s EnemyLocation=%s TargetLocation=%s Distance=%.1f AttackRange=%.1f"),
			*Enemy->GetName(), *Enemy->GetCombatTypeText().ToString(), *Enemy->GetActorLocation().ToCompactString(),
			*PlayerPawn->GetActorLocation().ToCompactString(), FMath::Sqrt(DistanceSquared), Enemy->AttackRange);
	}
}

UEnemyBTDecorator_HasTarget::UEnemyBTDecorator_HasTarget()
{
	NodeName = TEXT("Has Target");
}

bool UEnemyBTDecorator_HasTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	return AEnemyCharacter::IsValidCombatTarget(GetTarget(OwnerComp));
}

UEnemyBTDecorator_CanAttack::UEnemyBTDecorator_CanAttack()
{
	NodeName = TEXT("Can Start Attack");
}

bool UEnemyBTDecorator_CanAttack::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	return Enemy && Enemy->CanStartAttack(GetTarget(OwnerComp));
}

UEnemyBTDecorator_IsStaggered::UEnemyBTDecorator_IsStaggered()
{
	NodeName = TEXT("Is Staggered");
}

bool UEnemyBTDecorator_IsStaggered::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	return Enemy && Enemy->IsStaggered();
}

UEnemyBTDecorator_IsAlerting::UEnemyBTDecorator_IsAlerting()
{
	NodeName = TEXT("Is Alerting");
}

bool UEnemyBTDecorator_IsAlerting::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	return Enemy && Enemy->IsAlerting();
}

UEnemyBTDecorator_InCombatBand::UEnemyBTDecorator_InCombatBand()
{
	NodeName = TEXT("Cooling Down In Combat Band");
}

bool UEnemyBTDecorator_InCombatBand::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	const AActor* Target = GetTarget(OwnerComp);
	return Enemy && IsValid(Target) && Enemy->GetAttackCooldownRemaining() > 0.0f && Enemy->IsInCombatBand(Target);
}

UEnemyBTDecorator_HasLastKnownLocation::UEnemyBTDecorator_HasLastKnownLocation()
{
	NodeName = TEXT("Has Last Known Location");
}

bool UEnemyBTDecorator_HasLastKnownLocation::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	return Blackboard && !IsValid(GetTarget(OwnerComp))
		&& Blackboard->IsVectorValueSet(AEnemyAIController::LastKnownLocationKey);
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

	if (IsChaseComplete(Enemy, Target))
	{
		return EBTNodeResult::Succeeded;
	}

	Enemy->SetAIState(EEnemyAIState::Chase);
	const EPathFollowingRequestResult::Type Result = Controller->MoveToActor(Target, Enemy->GetChaseAcceptanceRadius(), false);
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

	if (IsChaseComplete(Enemy, Target))
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
	if (!Enemy || !Enemy->CanStartAttack(Target))
	{
		return EBTNodeResult::Failed;
	}
	if (AAIController* Controller = OwnerComp.GetAIOwner()) Controller->StopMovement();
	Enemy->SetAIState(EEnemyAIState::Attack);
	if (!Enemy->PerformAttack(Target)) return EBTNodeResult::Failed;

	// 모션이 끝나지 않는 경우를 대비한 안전 시간임
	reinterpret_cast<FTimedTaskMemory*>(NodeMemory)->RemainingTime = 6.0f;
	return EBTNodeResult::InProgress;
}

void UEnemyBTTask_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FTimedTaskMemory* Memory = reinterpret_cast<FTimedTaskMemory*>(NodeMemory);
	Memory->RemainingTime -= DeltaSeconds;
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	if (!Enemy)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 공격 모션과 돌진 착지가 끝나면 쿨다운은 거리 조절 분기에서 보냄
	const bool bLanded = !Enemy->GetCharacterMovement()->IsFalling();
	if ((!Enemy->IsAttackInProgress() && bLanded) || Memory->RemainingTime <= 0.0f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UEnemyBTTask_Attack::GetInstanceMemorySize() const
{
	return sizeof(FTimedTaskMemory);
}

UEnemyBTTask_MeleeAttack::UEnemyBTTask_MeleeAttack()
{
	NodeName = TEXT("Melee Attack: Weighted Pattern");
}

EBTNodeResult::Type UEnemyBTTask_MeleeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	return Enemy && Enemy->CombatType == EEnemyCombatType::Melee
		? Super::ExecuteTask(OwnerComp, NodeMemory)
		: EBTNodeResult::Failed;
}

UEnemyBTTask_RangedAttack::UEnemyBTTask_RangedAttack()
{
	NodeName = TEXT("Ranged Attack: Weighted Pattern");
}

EBTNodeResult::Type UEnemyBTTask_RangedAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	return Enemy && Enemy->CombatType == EEnemyCombatType::Ranged
		? Super::ExecuteTask(OwnerComp, NodeMemory)
		: EBTNodeResult::Failed;
}

UEnemyBTTask_Reposition::UEnemyBTTask_Reposition()
{
	NodeName = TEXT("Reposition: Hold / Strafe / Kite");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UEnemyBTTask_Reposition::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	AActor* Target = GetTarget(OwnerComp);
	if (!Controller || !Enemy || !IsValid(Target))
	{
		return EBTNodeResult::Failed;
	}
	if (Enemy->GetAttackCooldownRemaining() <= 0.0f)
	{
		return EBTNodeResult::Succeeded;
	}

	FRepositionMemory* Memory = reinterpret_cast<FRepositionMemory*>(NodeMemory);
	Memory->StrafeSign = FMath::RandBool() ? 1.0f : -1.0f;
	UpdateRepositionMove(Controller, Enemy, Target, Memory);
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UEnemyBTTask_Reposition::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* Controller = OwnerComp.GetAIOwner())
	{
		Controller->StopMovement();
	}
	return EBTNodeResult::Aborted;
}

void UEnemyBTTask_Reposition::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	AActor* Target = GetTarget(OwnerComp);
	if (!Controller || !Enemy || !IsValid(Target) || Enemy->IsStaggered() || !Enemy->IsInCombatBand(Target))
	{
		if (Controller) Controller->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	if (Enemy->GetAttackCooldownRemaining() <= 0.0f)
	{
		Controller->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	Enemy->FaceTarget(Target, DeltaSeconds);
	FRepositionMemory* Memory = reinterpret_cast<FRepositionMemory*>(NodeMemory);
	Memory->RepathTime -= DeltaSeconds;
	const bool bArrived = Enemy->CombatMovement != EEnemyCombatMovement::HoldGround
		&& Controller->GetMoveStatus() == EPathFollowingStatus::Idle;
	if (Memory->RepathTime <= 0.0f || bArrived)
	{
		UpdateRepositionMove(Controller, Enemy, Target, Memory);
	}
}

uint16 UEnemyBTTask_Reposition::GetInstanceMemorySize() const
{
	return sizeof(FRepositionMemory);
}

UEnemyBTTask_Alert::UEnemyBTTask_Alert()
{
	NodeName = TEXT("Alert");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UEnemyBTTask_Alert::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	if (!Enemy || !Enemy->IsAlerting())
	{
		return EBTNodeResult::Succeeded;
	}
	if (AAIController* Controller = OwnerComp.GetAIOwner()) Controller->StopMovement();
	Enemy->SetAIState(EEnemyAIState::Alert);
	return EBTNodeResult::InProgress;
}

void UEnemyBTTask_Alert::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	if (!Enemy || !Enemy->IsAlerting())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	Enemy->FaceTarget(GetTarget(OwnerComp), DeltaSeconds);
}

UEnemyBTTask_Stagger::UEnemyBTTask_Stagger()
{
	NodeName = TEXT("Stagger");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UEnemyBTTask_Stagger::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	if (!Enemy || !Enemy->IsStaggered())
	{
		return EBTNodeResult::Succeeded;
	}
	if (AAIController* Controller = OwnerComp.GetAIOwner()) Controller->StopMovement();
	return EBTNodeResult::InProgress;
}

void UEnemyBTTask_Stagger::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	const AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	if (!Enemy || !Enemy->IsStaggered())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

UEnemyBTTask_Search::UEnemyBTTask_Search()
{
	NodeName = TEXT("Search Last Known Location");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UEnemyBTTask_Search::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Controller || !Enemy || !Blackboard || IsValid(GetTarget(OwnerComp))
		|| !Blackboard->IsVectorValueSet(AEnemyAIController::LastKnownLocationKey))
	{
		return EBTNodeResult::Failed;
	}

	Enemy->SetAIState(EEnemyAIState::Search);
	FSearchMemory* Memory = reinterpret_cast<FSearchMemory*>(NodeMemory);
	Memory->LookTimeRemaining = Enemy->SearchDuration;
	Memory->NextTurnTime = 0.0f;
	Memory->LookYaw = Enemy->GetActorRotation().Yaw;
	const FVector Destination = Blackboard->GetValueAsVector(AEnemyAIController::LastKnownLocationKey);
	const EPathFollowingRequestResult::Type Result = MoveToNavigable(Controller, Destination);
	// 경로가 없거나 이미 도착했으면 그 자리에서 바로 둘러봄
	Memory->bLooking = Result != EPathFollowingRequestResult::RequestSuccessful;
	UE_LOG(LogTemp, Display, TEXT("ENEMY_SEARCH_START Enemy=%s Location=%s"), *Enemy->GetName(), *Destination.ToCompactString());
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UEnemyBTTask_Search::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* Controller = OwnerComp.GetAIOwner())
	{
		Controller->StopMovement();
	}
	return EBTNodeResult::Aborted;
}

void UEnemyBTTask_Search::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEnemyCharacter* Enemy = GetEnemy(OwnerComp);
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Controller || !Enemy || !Blackboard)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	if (IsValid(GetTarget(OwnerComp)))
	{
		Controller->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	FSearchMemory* Memory = reinterpret_cast<FSearchMemory*>(NodeMemory);
	if (!Memory->bLooking)
	{
		const FVector Destination = Blackboard->GetValueAsVector(AEnemyAIController::LastKnownLocationKey);
		const bool bArrived = FVector::DistSquared2D(Enemy->GetActorLocation(), Destination) <= FMath::Square(80.0f);
		if (bArrived || Controller->GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			Controller->StopMovement();
			Memory->bLooking = true;
		}
		return;
	}

	// 좌우로 크게 고개를 돌리며 둘러보고, 시간이 다 되면 수색을 포기함
	Memory->LookTimeRemaining -= DeltaSeconds;
	Memory->NextTurnTime -= DeltaSeconds;
	if (Memory->NextTurnTime <= 0.0f)
	{
		const float Swing = FMath::FRandRange(70.0f, 140.0f) * (FMath::RandBool() ? 1.0f : -1.0f);
		Memory->LookYaw = Enemy->GetActorRotation().Yaw + Swing;
		Memory->NextTurnTime = FMath::FRandRange(0.8f, 1.4f);
	}
	Enemy->TurnTowardsYaw(Memory->LookYaw, DeltaSeconds, 180.0f);
	if (Memory->LookTimeRemaining <= 0.0f)
	{
		Blackboard->ClearValue(AEnemyAIController::LastKnownLocationKey);
		UE_LOG(LogTemp, Display, TEXT("ENEMY_SEARCH_GAVE_UP Enemy=%s"), *Enemy->GetName());
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UEnemyBTTask_Search::GetInstanceMemorySize() const
{
	return sizeof(FSearchMemory);
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
