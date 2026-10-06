// Fill out your copyright notice in the Description page of Project Settings.


#include "LSW/Rooms/UT1_RewardRoom.h"
#include "LSW/Rooms/UT1_RewardChest.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AUT1_RewardRoom::AUT1_RewardRoom()
{
	PrimaryActorTick.bCanEverTick = false;

	ChestSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ChestSpawnPoint"));
	ChestSpawnPoint->SetupAttachment(RootComponent);
	ChestClass = AUT1_RewardChest::StaticClass();
}

void AUT1_RewardRoom::SetupRoom()
{
	Super::SetupRoom();
	ResetRoom();

	if (GetWorld() == nullptr || ChestClass == nullptr || ChestSpawnPoint == nullptr)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnedChest = GetWorld()->SpawnActor<AUT1_RewardChest>(
		ChestClass,
		ChestSpawnPoint->GetComponentTransform(),
		SpawnParams);
}

void AUT1_RewardRoom::ResetRoom()
{
	if (IsValid(SpawnedChest))
	{
		SpawnedChest->Destroy();
	}
	SpawnedChest = nullptr;
}

