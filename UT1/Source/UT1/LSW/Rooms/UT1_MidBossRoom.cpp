// Fill out your copyright notice in the Description page of Project Settings.


#include "LSW/Rooms/UT1_MidBossRoom.h"
#include "PYW/EnemyCharacter.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AUT1_MidBossRoom::AUT1_MidBossRoom()
{
    PrimaryActorTick.bCanEverTick = false;

    MidBossSpawnPoint =
        CreateDefaultSubobject<USceneComponent>(
            TEXT("MidBossSpawnPoint"));

    MidBossSpawnPoint->SetupAttachment(RootComponent);
}

void AUT1_MidBossRoom::BeginPlay()
{
    Super::BeginPlay();
}

void AUT1_MidBossRoom::SetupRoom()
{
    Super::SetupRoom();

    ResetRoom();
    SpawnMidBoss();
}

void AUT1_MidBossRoom::ResetRoom()
{
    if (SpawnedMidBoss)
    {
        SpawnedMidBoss->Destroy();
    }
}

void AUT1_MidBossRoom::SpawnMidBoss()
{
    if (!GetWorld() || !MidBossClass || !MidBossSpawnPoint)
        return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    SpawnedMidBoss =
        GetWorld()->SpawnActor<AEnemyCharacter>(
            MidBossClass,
            MidBossSpawnPoint->GetComponentLocation(),
            MidBossSpawnPoint->GetComponentRotation(),
            SpawnParams
        );
}