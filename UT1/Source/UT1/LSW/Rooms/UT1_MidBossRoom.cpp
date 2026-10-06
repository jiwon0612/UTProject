// Fill out your copyright notice in the Description page of Project Settings.


#include "LSW/Rooms/UT1_MidBossRoom.h"
#include "LSW/Rooms/UT1_RoomManager.h"
#include "CJW/Entities/UT1Entity.h"
#include "PYW/Entities/EnemyCharacter.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

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

    // 이미 클리어한 중간보스방에 돌아왔을 땐 보스를 다시 소환하지 않는다.
    if (AUT1_RoomManager* RoomManager = Cast<AUT1_RoomManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AUT1_RoomManager::StaticClass())))
    {
        const FRoomNode* CurrentNode = RoomManager->GetCurrentRoomNode();
        if (CurrentNode && CurrentNode->bIsCleared)
        {
            return;
        }
    }

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
    {
        UE_LOG(LogTemp, Error, TEXT("[MidBossRoom] Configure MidBossClass on the room Blueprint."));
        return;
    }

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

    if (SpawnedMidBoss)
    {
        SpawnedMidBoss->SetEnemyLevel(EnemyLevel);
    }

    if (SpawnedMidBoss)
    {
        SpawnedMidBoss->OnDied.AddDynamic(
            this,
            &AUT1_MidBossRoom::HandleMidBossDied
        );
    }
}

void AUT1_MidBossRoom::HandleMidBossDied(AUT1Entity* Entity)
{
    if (Entity != SpawnedMidBoss)
    {
        return;
    }

    if (AUT1_RoomManager* RoomManager = Cast<AUT1_RoomManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AUT1_RoomManager::StaticClass())))
    {
        RoomManager->MarkCurrentRoomCleared();
    }
}
