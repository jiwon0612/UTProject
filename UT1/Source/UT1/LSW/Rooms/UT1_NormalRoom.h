// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LSW/Rooms/UT1_RoomBase.h"
#include "UT1_NormalRoom.generated.h"

class AEnemyCharacter;
class USceneComponent;

UCLASS()
class UT1_API AUT1_NormalRoom : public AUT1_RoomBase
{
	GENERATED_BODY()
	
public:
	AUT1_NormalRoom();

private:
	void SpawnEnemies();

public:
	virtual void SetupRoom() override;
	virtual void ResetRoom() override;

public:
	int32 GetSpawnedEnemyCount() const
	{
		return SpawnedEnemies.Num();
	}

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TArray<TSubclassOf<AEnemyCharacter>> EnemyClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	int32 EnemyCount = 5;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	USceneComponent* EnemySpawnRoot;

private:
	UPROPERTY()
	TArray<AEnemyCharacter*> SpawnedEnemies;
};
