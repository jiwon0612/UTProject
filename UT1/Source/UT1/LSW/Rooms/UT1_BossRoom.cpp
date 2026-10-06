// Fill out your copyright notice in the Description page of Project Settings.


#include "LSW/Rooms/UT1_BossRoom.h"
#include "LSW/Rooms/UT1_RoomManager.h"
#include "CJW/Entities/UT1Entity.h"
#include "PYW/Entities/EnemyCharacter.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AUT1_BossRoom::AUT1_BossRoom()
{
	PrimaryActorTick.bCanEverTick = false;
	BossSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BossSpawnPoint"));
	BossSpawnPoint->SetupAttachment(RootComponent);
}

void AUT1_BossRoom::SetupRoom()
{
	Super::SetupRoom();
	SpawnBoss();
}

void AUT1_BossRoom::ResetRoom()
{
	if (IsValid(SpawnedBoss))
	{
		SpawnedBoss->Destroy();
		SpawnedBoss = nullptr;
	}
}

void AUT1_BossRoom::SpawnBoss()
{
	if (!GetWorld() || !BossSpawnPoint || !BossClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[BossRoom] Configure BossClass in the room Blueprint and ensure BossSpawnPoint exists."));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnedBoss = GetWorld()->SpawnActor<AEnemyCharacter>(BossClass,
		BossSpawnPoint->GetComponentLocation(), BossSpawnPoint->GetComponentRotation(), SpawnParams);
	if (!SpawnedBoss)
	{
		UE_LOG(LogTemp, Error, TEXT("[BossRoom] Failed to spawn boss class %s."), *GetNameSafe(BossClass));
		return;
	}

	SpawnedBoss->SetEnemyLevel(EnemyLevel);
	SpawnedBoss->OnDied.AddDynamic(this, &AUT1_BossRoom::HandleBossDied);
}

void AUT1_BossRoom::HandleBossDied(AUT1Entity* Entity)
{
	if (Entity != SpawnedBoss)
	{
		return;
	}

	if (AUT1_RoomManager* RoomManager = Cast<AUT1_RoomManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AUT1_RoomManager::StaticClass())))
	{
		RoomManager->ShowGameClear();
	}
}

