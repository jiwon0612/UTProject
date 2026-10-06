// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UT1LootDropComponent.generated.h"

class AUT1Entity;
class AUT1LootPickup;
class UUT1MaterialData;
class UUT1WeaponData;

/** 재료 드랍 한 줄: Chance 확률로 MinCount~MaxCount 개. */
USTRUCT(BlueprintType)
struct FUT1MaterialDrop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UUT1MaterialData> Material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 MaxCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 1.0f;
};

/** 설계도 드랍 한 줄: Chance 확률로 Weapon 의 설계도. */
USTRUCT(BlueprintType)
struct FUT1BlueprintDrop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UUT1WeaponData> Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 0.1f;
};

/**
 * 무엇을 얼마나 떨굴지 정하고 AUT1LootPickup 을 뿌리는 컴포넌트.
 *
 * 적 클래스에 드랍 코드를 넣지 않고 컴포넌트로 뺀 이유:
 *   - 적(PYW 의 AEnemyCharacter), 허수아비, 보스가 같은 드랍 규칙을 쓴다.
 *   - AUT1Entity 가 공개하는 OnDied 만 구독하므로 적 클래스 코드를 고치지 않고
 *     BP 에 컴포넌트를 붙이는 것만으로 드랍이 생긴다.
 *   - 방 클리어 보상처럼 "죽음"이 아닌 시점에도 DropLoot 를 직접 부르면 된다.
 */
UCLASS(ClassGroup = (UT1), meta = (BlueprintSpawnableComponent))
class UT1_API UUT1LootDropComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUT1LootDropComponent();

	// 드랍 테이블을 굴려서 주인 발밑 주변에 픽업을 뿌린다.
	// 방 클리어 보상 등에서 직접 부를 수 있게 공개한다.
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void DropLoot();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TArray<FUT1MaterialDrop> MaterialDrops;

	// 한 번에 설계도는 최대 1장만 떨어진다. 위에서부터 굴려서 처음 당첨된 것 하나.
	// 플레이어가 이미 해금한 설계도는 건너뛴다 (중복 드랍 방지).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TArray<FUT1BlueprintDrop> BlueprintDrops;

	// 끄면 죽어도 떨구지 않는다. 방 보상처럼 DropLoot 를 직접 부르는 용도.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	bool bDropOnOwnerDeath = true;

	// 꾸민 픽업 BP 를 쓰려면 여기서 바꾼다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TSubclassOf<AUT1LootPickup> PickupClass;

	// 픽업이 한 점에 겹치지 않게 주인 주변 이 반경 안에 흩뿌린다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0.0"))
	float ScatterRadius = 90.0f;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOwnerDied(AUT1Entity* Entity);

	// 주인 아래 바닥 높이. 공중에 뜬 적이 죽어도 픽업은 바닥에 놓인다.
	FVector FindDropOrigin() const;

	AUT1LootPickup* SpawnPickup(const FVector& Location, TFunctionRef<void(AUT1LootPickup*)> Init);
};
