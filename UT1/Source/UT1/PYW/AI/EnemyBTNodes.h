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
class UT1_API UEnemyBTDecorator_IsStaggered : public UBTDecorator
{
	GENERATED_BODY()

public:
	UEnemyBTDecorator_IsStaggered();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};

UCLASS()
class UT1_API UEnemyBTDecorator_IsAlerting : public UBTDecorator
{
	GENERATED_BODY()

public:
	UEnemyBTDecorator_IsAlerting();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};

/** 공격 쿨다운 중이고 대상이 교전 거리 안이면 추격 대신 거리 조절을 함 */
UCLASS()
class UT1_API UEnemyBTDecorator_InCombatBand : public UBTDecorator
{
	GENERATED_BODY()

public:
	UEnemyBTDecorator_InCombatBand();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};

/** 대상을 놓쳤고 마지막으로 본 위치가 남아 있으면 수색함 */
UCLASS()
class UT1_API UEnemyBTDecorator_HasLastKnownLocation : public UBTDecorator
{
	GENERATED_BODY()

public:
	UEnemyBTDecorator_HasLastKnownLocation();

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

/** Attack branch used only by melee enemies in the runtime Behavior Tree. */
UCLASS()
class UT1_API UEnemyBTTask_MeleeAttack : public UEnemyBTTask_Attack
{
	GENERATED_BODY()

public:
	UEnemyBTTask_MeleeAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

/** Attack branch used only by ranged enemies in the runtime Behavior Tree. */
UCLASS()
class UT1_API UEnemyBTTask_RangedAttack : public UEnemyBTTask_Attack
{
	GENERATED_BODY()

public:
	UEnemyBTTask_RangedAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

/** 쿨다운 동안 CombatMovement에 따라 압박, 선회, 후퇴함 */
UCLASS()
class UT1_API UEnemyBTTask_Reposition : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEnemyBTTask_Reposition();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
};

UCLASS()
class UT1_API UEnemyBTTask_Alert : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEnemyBTTask_Alert();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

UCLASS()
class UT1_API UEnemyBTTask_Stagger : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEnemyBTTask_Stagger();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

/** 마지막으로 본 위치까지 이동한 뒤 주위를 둘러보고, 못 찾으면 순찰로 돌아감 */
UCLASS()
class UT1_API UEnemyBTTask_Search : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEnemyBTTask_Search();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
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

