// Fill out your copyright notice in the Description page of Project Settings.


#include "LSW/Rooms/UT1_RoomBase.h"
#include "Components/SceneComponent.h"

AUT1_RoomBase::AUT1_RoomBase()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(
            TEXT("Root"));

    PlayerSpawnPoint =
        CreateDefaultSubobject<USceneComponent>(
            TEXT("PlayerSpawnPoint")
        );

    PlayerSpawnPoint->SetupAttachment(
        RootComponent
    );
}

void AUT1_RoomBase::BeginPlay()
{
    Super::BeginPlay();
}

void AUT1_RoomBase::SetupRoom()
{

}

FVector AUT1_RoomBase::GetPlayerSpawnLocation() const
{
    if (PlayerSpawnPoint)
    {
        return PlayerSpawnPoint->GetComponentLocation();
    }

    return GetActorLocation();
}