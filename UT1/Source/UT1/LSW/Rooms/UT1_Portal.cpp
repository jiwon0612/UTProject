#include "LSW/Rooms/UT1_Portal.h"
#include "LSW/Rooms/UT1_RoomManager.h"
#include "CJW/Player/UT1Player.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AUT1_Portal::AUT1_Portal()
{
    PrimaryActorTick.bCanEverTick = false;

    InteractRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractRange"));
    SetRootComponent(InteractRange);
    InteractRange->SetSphereRadius(220.0f);
    InteractRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AUT1_Portal::Interact_Implementation(AUT1Player* Interactor)
{
    if (Interactor == nullptr || !IsPlayerInRange())
    {
        return;
    }

    if (AUT1_RoomManager* RoomManager = Cast<AUT1_RoomManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AUT1_RoomManager::StaticClass())))
    {
        RoomManager->TryInteractPortal();
    }
}
