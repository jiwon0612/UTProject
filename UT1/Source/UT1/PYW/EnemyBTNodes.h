#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnemyBTNodes.generated.h"

UCLASS()
class UT1_API UEnemyBTService_FindTarget : public UBTService
{
	GENERATED_BODY()

public:
	UEnemyBTService_FindTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnSearchStart(FBehaviorTreeSearchData& SearchData) override;

private:
	void UpdateTarget(UBehaviorTreeComponent& OwnerComp) const;
};

UCLASS()
class UT1_API UEnemyBTDecorator_HasTarget : public UBTDecorator
{
	GENERATED_BODY()

public:
	UEnemyBTDecorator_HasTarget();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};

UCLASS()
class UT1_API UEnemyBTDecorator_CanAttack : public UBTDecorator
{
	GENERATED_BODY()

public:
	UEnemyBTDecorator_CanAttack();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};

UCLASS()
class UT1_API UEnemyBTTask_FindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEnemyBTTask_FindPatrolPoint();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class UT1_API UEnemyBTTask_WalkPatrol : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEnemyBTTask_WalkPatrol();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

UCLASS()
class UT1_API UEnemyBTTask_Chase : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEnemyBTTask_Chase();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

UCLASS()
class UT1_API UEnemyBTTask_Attack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEnemyBTTask_Attack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
};

UCLASS()
class UT1_API UEnemyBTTask_Idle : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEnemyBTTask_Idle();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
};


