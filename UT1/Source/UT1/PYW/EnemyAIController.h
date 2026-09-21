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


