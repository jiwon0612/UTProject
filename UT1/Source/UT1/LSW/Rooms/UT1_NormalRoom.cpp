// Fill out your copyright notice in the Description page of Project Settings.


#include "LSW/Rooms/UT1_NormalRoom.h"
#include "Engine/World.h"
#include "PYW/EnemyCharacter.h"

AUT1_NormalRoom::AUT1_NormalRoom()
{
    PrimaryActorTick.bCanEverTick = false;

    EnemySpawnRoot =
        CreateDefaultSubobject<USceneComponent>(
            TEXT("EnemySpawnRoot"));

    EnemySpawnRoot->SetupAttachment(RootComponent);
}

void AUT1_NormalRoom::SetupRoom()
{
    ResetRoom();
    SpawnEnemies();
}

void AUT1_NormalRoom::ResetRoom()
{
    for (AEnemyCharacter* Enemy : SpawnedEnemies)
    {
        if (IsValid(Enemy))
        {
            Enemy->Destroy();
        }
    }

    SpawnedEnemies.Empty();
}

void AUT1_NormalRoom::SpawnEnemies()
{
    if (!GetWorld() || !EnemySpawnRoot)
        return;

    if (EnemyClasses.Num() == 0)
        return;

    TArray<USceneComponent*> SpawnPoints;

    EnemySpawnRoot->GetChildrenComponents(true, SpawnPoints);

    const int32 SpawnCount =
        FMath::Min(EnemyCount, SpawnPoints.Num());

    for (int32 i = 0; i < SpawnCount; ++i)
    {
        USceneComponent* SpawnPoint = SpawnPoints[i];

        if (!SpawnPoint)
            continue;

        int32 ClassIndex =
            FMath::RandRange(0, EnemyClasses.Num() - 1);

        TSubclassOf<AEnemyCharacter> EnemyClass =
            EnemyClasses[ClassIndex];

        if (!EnemyClass)
            continue;

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AEnemyCharacter* Enemy =
            GetWorld()->SpawnActor<AEnemyCharacter>(
                EnemyClass,
                SpawnPoint->GetComponentLocation(),
                SpawnPoint->GetComponentRotation(),
                SpawnParams);

        if (Enemy)
        {
            SpawnedEnemies.Add(Enemy);
        }
    }
}