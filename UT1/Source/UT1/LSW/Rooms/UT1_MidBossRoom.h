// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LSW/Rooms/UT1_RoomBase.h"
#include "UT1_MidBossRoom.generated.h"

class AEnemyCharacter;

UCLASS()
class UT1_API AUT1_MidBossRoom : public AUT1_RoomBase
{
	GENERATED_BODY()
	
public:
    AUT1_MidBossRoom();

protected:
    virtual void BeginPlay() override;

public:
    virtual void SetupRoom() override;
    virtual void ResetRoom() override;

private:
    void SpawnMidBoss();
    void GiveMidBossReward();

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBoss")
    USceneComponent* MidBossSpawnPoint;

private:
    UPROPERTY(EditAnywhere, Category = "MidBoss")
    TSubclassOf<AEnemyCharacter> MidBossClass;

    UPROPERTY(EditAnywhere, Category = "MidBoss")
    int32 RewardAmount = 3;

    UPROPERTY()
    AEnemyCharacter* SpawnedMidBoss = nullptr;
};
