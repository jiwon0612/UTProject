// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UT1LootPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UUT1MaterialData;
class UUT1WeaponData;

/**
 * 바닥에 떨어진 재료 또는 설계도. 플레이어가 닿으면 런 인벤토리에 들어가고 사라진다.
 *
 * 재료와 설계도를 한 클래스로 처리한다. 줍는 방식(닿으면 획득)이 같고,
 * 차이는 "무엇을 넣느냐" 한 줄뿐이라 클래스를 나눌 이득이 없다.
 *
 * 두 가지 방식으로 쓴다.
 *   - 드랍: UUT1LootDropComponent 가 스폰하면서 InitMaterial / InitBlueprint 로 채운다.
 *   - 배치: 레벨에 직접 놓고 디테일 패널에서 Material/Count 또는 BlueprintWeapon 을 고른다.
 *     (방 보상 상자 안에 미리 놓아 두는 식)
 */
UCLASS()
class UT1_API AUT1LootPickup : public AActor
{
	GENERATED_BODY()

public:
	AUT1LootPickup();

	// SpawnActorDeferred 와 FinishSpawning 사이에 부른다. 그래야 스폰 순간
	// 플레이어와 겹쳐 바로 주워질 때도 내용물이 비어 있지 않다.
	void InitMaterial(UUT1MaterialData* InMaterial, int32 InCount);
	void InitBlueprint(UUT1WeaponData* InWeapon);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USphereComponent> PickupRange;

	// 기본은 엔진 기본 도형(재료: 구, 설계도: 큐브). BP 에서 원하는 메시로 바꾼다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// "고철 x3" / "설계도: 대검". 무엇이 떨어졌는지 바로 보이게.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UTextRenderComponent> Label;

	// --- 내용물. 둘 중 하나만 채운다. BlueprintWeapon 이 있으면 설계도로 취급한다 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UUT1MaterialData> Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UUT1WeaponData> BlueprintWeapon;

private:
	// 내용물에 맞춰 라벨과 모양을 바꾼다.
	void RefreshVisual();

	UPROPERTY()
	TObjectPtr<UStaticMesh> MaterialShape;

	UPROPERTY()
	TObjectPtr<UStaticMesh> BlueprintShape;

	// 같은 프레임에 겹침이 두 번 들어와도 한 번만 지급한다.
	bool bCollected = false;
};
