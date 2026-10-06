// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LSW/Rooms/UT1_RoomBase.h"
#include "UT1_BossRoom.generated.h"

class AEnemyCharacter;
class AUT1Entity;
class USceneComponent;

/**
 * 
 */
UCLASS()
class UT1_API AUT1_BossRoom : public AUT1_RoomBase
{
	GENERATED_BODY()

public:
	AUT1_BossRoom();
	virtual void SetupRoom() override;
	virtual void ResetRoom() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<USceneComponent> BossSpawnPoint;

	// BP_GuardianEnemy 같은 적 블루프린트를 에디터에서 최종 보스로 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	TSubclassOf<AEnemyCharacter> BossClass;

private:
	void SpawnBoss();

	UFUNCTION()
	void HandleBossDied(AUT1Entity* Entity);

	UPROPERTY()
	TObjectPtr<AEnemyCharacter> SpawnedBoss;
};
