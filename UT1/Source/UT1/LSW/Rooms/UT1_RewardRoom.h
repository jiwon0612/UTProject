// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LSW/Rooms/UT1_RoomBase.h"
#include "UT1_RewardRoom.generated.h"

class AUT1_RewardChest;
class USceneComponent;

UCLASS()
class UT1_API AUT1_RewardRoom : public AUT1_RoomBase
{
	GENERATED_BODY()

public:
	AUT1_RewardRoom();

	virtual void SetupRoom() override;
	virtual void ResetRoom() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward")
	TObjectPtr<USceneComponent> ChestSpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TSubclassOf<AUT1_RewardChest> ChestClass;

private:
	UPROPERTY()
	TObjectPtr<AUT1_RewardChest> SpawnedChest;
};
