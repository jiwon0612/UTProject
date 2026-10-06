#include "PYW/AI/EnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "PYW/AI/EnemyBTNodes.h"
#include "PYW/Entities/EnemyCharacter.h"

const FName AEnemyAIController::TargetActorKey(TEXT("TargetActor"));
const FName AEnemyAIController::PatrolLocationKey(TEXT("PatrolLocation"));
const FName AEnemyAIController::LastKnownLocationKey(TEXT("LastKnownLocation"));

namespace
{
	FBTCompositeChild& AddCompositeChild(UBTCompositeNode* Parent, UBTCompositeNode* Child)
	{
		FBTCompositeChild& Entry = Parent->Children.AddDefaulted_GetRef();
		Entry.ChildComposite = Child;
		return Entry;
	}

	FBTCompositeChild& AddTaskChild(UBTCompositeNode* Parent, UBTTaskNode* Child)
	{
		FBTCompositeChild& Entry = Parent->Children.AddDefaulted_GetRef();
		Entry.ChildTask = Child;
		return Entry;
	}

	UBTComposite_Sequence* AddSequenceBranch(UBehaviorTree* Tree, UBTCompositeNode* Root, const TCHAR* ObjectName,
		const TCHAR* NodeName, UBTDecorator* Decorator)
	{
		UBTComposite_Sequence* Sequence = NewObject<UBTComposite_Sequence>(Tree, ObjectName);
		Sequence->NodeName = NodeName;
		FBTCompositeChild& Branch = AddCompositeChild(Root, Sequence);
		if (Decorator) Branch.Decorators.Add(Decorator);
		return Sequence;
	}
}

AEnemyAIController::AEnemyAIController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("EnemyBlackboard"));
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("EnemyBehaviorTree"));
	BrainComponent = BehaviorTreeComponent;
	bAttachToPawn = true;
	bStartAILogicOnPossess = true;
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	BuildRuntimeBehaviorTree();

	UBlackboardComponent* LocalBlackboard = BlackboardComponent.Get();
	if (RuntimeBlackboard && RuntimeBehaviorTree && UseBlackboard(RuntimeBlackboard, LocalBlackboard))
	{
		BlackboardComponent = LocalBlackboard;
		BehaviorTreeComponent->StartTree(*RuntimeBehaviorTree);
	}
}

void AEnemyAIController::SetTargetActor(AActor* NewTarget)
{
	UBlackboardComponent* LocalBlackboard = GetBlackboardComponent();
	if (!LocalBlackboard || !IsValid(NewTarget))
	{
		return;
	}
	// 맞은 순간을 목격한 것으로 보고 시야 타이머와 마지막 위치를 갱신함
	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn())) Enemy->MarkTargetSeen();
	LocalBlackboard->SetValueAsVector(LastKnownLocationKey, NewTarget->GetActorLocation());
	if (LocalBlackboard->GetValueAsObject(TargetActorKey) == NewTarget)
	{
		return;
	}
	LocalBlackboard->SetValueAsObject(TargetActorKey, NewTarget);
	UE_LOG(LogTemp, Display, TEXT("ENEMY_TARGET_AGGRO Enemy=%s Target=%s"), *GetNameSafe(GetPawn()), *GetNameSafe(NewTarget));
}

void AEnemyAIController::RestartBehavior()
{
	if (BrainComponent) BrainComponent->RestartLogic();
}

void AEnemyAIController::BuildRuntimeBehaviorTree()
{
	if (RuntimeBehaviorTree)
	{
		return;
	}

	RuntimeBlackboard = NewObject<UBlackboardData>(this, TEXT("Enemy_RuntimeBlackboard"));
	UBlackboardKeyType_Object* TargetKeyType = RuntimeBlackboard->UpdatePersistentKey<UBlackboardKeyType_Object>(TargetActorKey);
	TargetKeyType->BaseClass = AActor::StaticClass();
	RuntimeBlackboard->UpdatePersistentKey<UBlackboardKeyType_Vector>(PatrolLocationKey);
	RuntimeBlackboard->UpdatePersistentKey<UBlackboardKeyType_Vector>(LastKnownLocationKey);

	RuntimeBehaviorTree = NewObject<UBehaviorTree>(this, TEXT("Enemy_RuntimeBehaviorTree"));
	RuntimeBehaviorTree->BlackboardAsset = RuntimeBlackboard;

	UBTComposite_Selector* Root = NewObject<UBTComposite_Selector>(RuntimeBehaviorTree, TEXT("RootSelector"));
	Root->NodeName = TEXT("Enemy: Stagger > Alert > Attack > Reposition > Chase > Search > Patrol");
	Root->Services.Add(NewObject<UEnemyBTService_FindTarget>(RuntimeBehaviorTree, TEXT("FindTargetService")));
	RuntimeBehaviorTree->RootNode = Root;
	UBehaviorTree* Tree = RuntimeBehaviorTree;

	UBTComposite_Sequence* StaggerSequence = AddSequenceBranch(Tree, Root, TEXT("StaggerSequence"), TEXT("Stagger"),
		NewObject<UEnemyBTDecorator_IsStaggered>(Tree, TEXT("IsStaggered")));
	AddTaskChild(StaggerSequence, NewObject<UEnemyBTTask_Stagger>(Tree, TEXT("StaggerTask")));

	UBTComposite_Sequence* AlertSequence = AddSequenceBranch(Tree, Root, TEXT("AlertSequence"), TEXT("Alert"),
		NewObject<UEnemyBTDecorator_IsAlerting>(Tree, TEXT("IsAlerting")));
	AddTaskChild(AlertSequence, NewObject<UEnemyBTTask_Alert>(Tree, TEXT("AlertTask")));

	const AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());
	const bool bIsRangedEnemy = Enemy && Enemy->CombatType == EEnemyCombatType::Ranged;
	UBTComposite_Sequence* AttackSequence = AddSequenceBranch(Tree, Root, TEXT("AttackSequence"),
		bIsRangedEnemy ? TEXT("Ranged Attack") : TEXT("Melee Attack"),
		NewObject<UEnemyBTDecorator_CanAttack>(Tree, TEXT("CanAttack")));
	if (bIsRangedEnemy)
	{
		AddTaskChild(AttackSequence, NewObject<UEnemyBTTask_RangedAttack>(Tree, TEXT("RangedAttackTask")));
	}
	else
	{
		AddTaskChild(AttackSequence, NewObject<UEnemyBTTask_MeleeAttack>(Tree, TEXT("MeleeAttackTask")));
	}
	UE_LOG(LogTemp, Display, TEXT("ENEMY_BT_ATTACK_BRANCH Enemy=%s Branch=%s"),
		*GetNameSafe(Enemy), bIsRangedEnemy ? TEXT("RangedAttack") : TEXT("MeleeAttack"));

	UBTComposite_Sequence* RepositionSequence = AddSequenceBranch(Tree, Root, TEXT("RepositionSequence"), TEXT("Reposition"),
		NewObject<UEnemyBTDecorator_InCombatBand>(Tree, TEXT("InCombatBand")));
	AddTaskChild(RepositionSequence, NewObject<UEnemyBTTask_Reposition>(Tree, TEXT("RepositionTask")));

	UBTComposite_Sequence* ChaseSequence = AddSequenceBranch(Tree, Root, TEXT("ChaseSequence"), TEXT("Chase"),
		NewObject<UEnemyBTDecorator_HasTarget>(Tree, TEXT("HasTarget")));
	AddTaskChild(ChaseSequence, NewObject<UEnemyBTTask_Chase>(Tree, TEXT("ChaseTask")));

	UBTComposite_Sequence* SearchSequence = AddSequenceBranch(Tree, Root, TEXT("SearchSequence"), TEXT("Search"),
		NewObject<UEnemyBTDecorator_HasLastKnownLocation>(Tree, TEXT("HasLastKnownLocation")));
	AddTaskChild(SearchSequence, NewObject<UEnemyBTTask_Search>(Tree, TEXT("SearchTask")));

	UBTComposite_Sequence* PatrolSequence = AddSequenceBranch(Tree, Root, TEXT("PatrolSequence"), TEXT("Walk / Idle"), nullptr);
	AddTaskChild(PatrolSequence, NewObject<UEnemyBTTask_FindPatrolPoint>(Tree, TEXT("FindPatrolPoint")));
	AddTaskChild(PatrolSequence, NewObject<UEnemyBTTask_WalkPatrol>(Tree, TEXT("WalkTask")));
	AddTaskChild(PatrolSequence, NewObject<UEnemyBTTask_Idle>(Tree, TEXT("IdleTask")));
}
