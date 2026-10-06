// Fill out your copyright notice in the Description page of Project Settings.


#include "LSW/Rooms/UT1_NormalRoom.h"
#include "LSW/Rooms/UT1_RoomManager.h"
#include "CJW/Entities/UT1Entity.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PYW/Entities/EnemyCharacter.h"

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

    for (int32 Index = SpawnPoints.Num() - 1; Index > 0; --Index)
    {
        const int32 SwapIndex = FMath::RandRange(0, Index);
        SpawnPoints.Swap(Index, SwapIndex);
    }

    TArray<TSubclassOf<AEnemyCharacter>> ValidEnemyClasses;
    for (const TSubclassOf<AEnemyCharacter>& EnemyClass : EnemyClasses)
    {
        if (EnemyClass)
        {
            ValidEnemyClasses.Add(EnemyClass);
        }
    }

    if (ValidEnemyClasses.IsEmpty())
    {
        return;
    }

    const int32 SpawnCount =
        FMath::Min(EnemyCount, SpawnPoints.Num());

    for (int32 i = 0; i < SpawnCount; ++i)
    {
        USceneComponent* SpawnPoint = SpawnPoints[i];

        if (!SpawnPoint)
            continue;

        TSubclassOf<AEnemyCharacter> EnemyClass =
            ValidEnemyClasses[FMath::RandRange(0, ValidEnemyClasses.Num() - 1)];

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
            Enemy->SetEnemyLevel(EnemyLevel);
            Enemy->SetRoomDifficultyMultiplier(EnemyDifficultyMultiplier);
            SpawnedEnemies.Add(Enemy);
            Enemy->OnDied.AddDynamic(this, &AUT1_NormalRoom::HandleEnemyDied);
        }
    }
}

void AUT1_NormalRoom::HandleEnemyDied(AUT1Entity* Entity)
{
    AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Entity);
    if (!Enemy || SpawnedEnemies.Remove(Enemy) == 0)
    {
        return;
    }

    if (!SpawnedEnemies.IsEmpty())
    {
        return;
    }

    if (AUT1_RoomManager* RoomManager = Cast<AUT1_RoomManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AUT1_RoomManager::StaticClass())))
    {
        RoomManager->MarkCurrentRoomCleared();
    }
}
