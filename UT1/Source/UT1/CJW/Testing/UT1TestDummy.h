// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CJW/Entities/UT1Entity.h"
#include "UT1TestDummy.generated.h"

class UStaticMeshComponent;

/**
 * 타격 판정과 피격 로직 확인용 허수아비.
 *
 * AUT1Entity 를 상속해서 체력과 사망 처리를 그대로 쓴다. 적이 생기기 전까지
 * 전투 사슬(무기 트레이스 -> ApplyPointDamage -> TakeDamage -> 체력 -> 사망)을
 * 끝에서 끝까지 확인할 수 있는 유일한 대상이다.
 *
 * Weapon 채널 응답을 생성자에서 Overlap 으로 켜 둔다. Block 이 아닌 이유는
 * SweepMulti 가 블로킹 대상에서 멈춰, 겹쳐 선 둘 중 앞사람만 맞기 때문이다.
 *
 * 임시 검증용이다. 적이 들어오면 지우거나 Developer 모듈로 옮길 것.
 */
UCLASS()
class UT1_API AUT1TestDummy : public AUT1Entity
{
	GENERATED_BODY()

public:
	AUT1TestDummy();

	// 스켈레탈 메시가 없으므로 큐브로 대신 보여 준다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dummy")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	// 죽으면 드랍한다. 테이블은 레벨에 배치한 인스턴스의 디테일 패널에서 채운다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dummy")
	TObjectPtr<class UUT1LootDropComponent> LootDrop;

protected:
	virtual void HandleDamaged(float ActualDamage, AActor* DamageCauser) override;
	virtual void HandleDeath(AActor* Killer) override;
};
