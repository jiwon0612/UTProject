#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UBehaviorTree;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UBlackboardData;

UCLASS()
class UT1_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	static const FName TargetActorKey;
	static const FName PatrolLocationKey;
	static const FName LastKnownLocationKey;

	void SetTargetActor(AActor* NewTarget);

	/** 진행 중인 태스크를 끊고 루트부터 다시 분기를 고름. 경직처럼 즉시 반응해야 할 때 사용함 */
	void RestartBehavior();

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	void BuildRuntimeBehaviorTree();

	UPROPERTY(VisibleAnywhere, Category = "Enemy|AI")
	TObjectPtr<UBlackboardComponent> BlackboardComponent;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|AI")
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent;

	UPROPERTY(Transient)
	TObjectPtr<UBlackboardData> RuntimeBlackboard;

	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTree> RuntimeBehaviorTree;
};


