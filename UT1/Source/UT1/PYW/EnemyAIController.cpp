#include "PYW/EnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "PYW/EnemyBTNodes.h"

const FName AEnemyAIController::TargetActorKey(TEXT("TargetActor"));
const FName AEnemyAIController::PatrolLocationKey(TEXT("PatrolLocation"));

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

	RuntimeBehaviorTree = NewObject<UBehaviorTree>(this, TEXT("Enemy_RuntimeBehaviorTree"));
	RuntimeBehaviorTree->BlackboardAsset = RuntimeBlackboard;

	UBTComposite_Selector* Root = NewObject<UBTComposite_Selector>(RuntimeBehaviorTree, TEXT("RootSelector"));
	Root->NodeName = TEXT("Enemy: Attack > Chase > Patrol");
	Root->Services.Add(NewObject<UEnemyBTService_FindTarget>(RuntimeBehaviorTree, TEXT("FindTargetService")));
	RuntimeBehaviorTree->RootNode = Root;

	UBTComposite_Sequence* AttackSequence = NewObject<UBTComposite_Sequence>(RuntimeBehaviorTree, TEXT("AttackSequence"));
	AttackSequence->NodeName = TEXT("Attack");
	FBTCompositeChild& AttackBranch = AddCompositeChild(Root, AttackSequence);
	AttackBranch.Decorators.Add(NewObject<UEnemyBTDecorator_CanAttack>(RuntimeBehaviorTree, TEXT("CanAttack")));
	AddTaskChild(AttackSequence, NewObject<UEnemyBTTask_Attack>(RuntimeBehaviorTree, TEXT("AttackTask")));

	UBTComposite_Sequence* ChaseSequence = NewObject<UBTComposite_Sequence>(RuntimeBehaviorTree, TEXT("ChaseSequence"));
	ChaseSequence->NodeName = TEXT("Chase");
	FBTCompositeChild& ChaseBranch = AddCompositeChild(Root, ChaseSequence);
	ChaseBranch.Decorators.Add(NewObject<UEnemyBTDecorator_HasTarget>(RuntimeBehaviorTree, TEXT("HasTarget")));
	AddTaskChild(ChaseSequence, NewObject<UEnemyBTTask_Chase>(RuntimeBehaviorTree, TEXT("ChaseTask")));

	UBTComposite_Sequence* PatrolSequence = NewObject<UBTComposite_Sequence>(RuntimeBehaviorTree, TEXT("PatrolSequence"));
	PatrolSequence->NodeName = TEXT("Walk / Idle");
	AddCompositeChild(Root, PatrolSequence);
	AddTaskChild(PatrolSequence, NewObject<UEnemyBTTask_FindPatrolPoint>(RuntimeBehaviorTree, TEXT("FindPatrolPoint")));
	AddTaskChild(PatrolSequence, NewObject<UEnemyBTTask_WalkPatrol>(RuntimeBehaviorTree, TEXT("WalkTask")));
	AddTaskChild(PatrolSequence, NewObject<UEnemyBTTask_Idle>(RuntimeBehaviorTree, TEXT("IdleTask")));
}
