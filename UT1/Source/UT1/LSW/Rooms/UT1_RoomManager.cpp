#include "LSW/Rooms/UT1_RoomManager.h"
#include "LSW/Rooms/UT1_RoomBase.h"
#include "LSW/Rooms/UT1_Portal.h"
#include "LSW/Widget/UT1_PortalWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

AUT1_RoomManager::AUT1_RoomManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AUT1_RoomManager::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PlayerController =
        UGameplayStatics::GetPlayerController(
            GetWorld(),
            0
        );

    if (PlayerController)
    {
        EnableInput(PlayerController);

        if (InputComponent)
        {
            InputComponent->BindKey(
                EKeys::E,
                IE_Pressed,
                this,
                &AUT1_RoomManager::TryInteractPortal
            );
        }
    }

    GenerateDungeon();
}

void AUT1_RoomManager::GenerateDungeon()
{
    RoomNodes.Empty();

    if (DungeonRoomCount <= 0)
    {
        return;
    }

    if (!BaseRoomClass)
    {
        return;
    }

    if (NormalRoomClasses.Num() == 0)
    {
        return;
    }

    FRoomNode BaseRoom;

    BaseRoom.RoomID = 0;
    BaseRoom.RoomType = ERoomType::Base;
    BaseRoom.bIsCleared = true;
    BaseRoom.SelectedRoomIndex = INDEX_NONE;

    RoomNodes.Add(BaseRoom);

    bool bPreviousWasMidBoss = false;

    for (int32 i = 0; i < DungeonRoomCount; ++i)
    {
        const int32 RoomID = i + 1;

        FRoomNode NewRoom;

        NewRoom.RoomID = RoomID;
        NewRoom.bIsCleared = false;
        NewRoom.SelectedRoomIndex = INDEX_NONE;

        if (RoomID == DungeonRoomCount)
        {
            NewRoom.RoomType = ERoomType::Boss;

            NewRoom.SelectedRoomIndex =
                GetRandomRoomIndex(BossRoomClasses);

            bPreviousWasMidBoss = false;
        }
        else
        {
            NewRoom.RoomType =
                GetRandomRoomType(RoomID, bPreviousWasMidBoss);

            switch (NewRoom.RoomType)
            {
                case ERoomType::Normal:
                {
                    NewRoom.SelectedRoomIndex =
                        GetRandomRoomIndex(NormalRoomClasses);

                    break;
                }

                case ERoomType::Gimmick:
                {
                    NewRoom.SelectedRoomIndex = GetRandomRoomIndex(GimmickRoomClasses);

                    if (NewRoom.SelectedRoomIndex == INDEX_NONE)
                    {
                        NewRoom.RoomType = ERoomType::Normal;
                        NewRoom.SelectedRoomIndex = GetRandomRoomIndex(NormalRoomClasses);
                        bPreviousWasMidBoss = false;
                    }

                    break;
                }

                case ERoomType::MidBoss:
                {
                    NewRoom.SelectedRoomIndex =
                        GetRandomRoomIndex(
                            MidBossRoomClasses
                        );

                    if (NewRoom.SelectedRoomIndex == INDEX_NONE)
                    {
                        NewRoom.RoomType = ERoomType::Normal;

                        NewRoom.SelectedRoomIndex =
                            GetRandomRoomIndex(NormalRoomClasses);

                        bPreviousWasMidBoss = false;
                    }

                    break;
                }

                default:
                {
                    NewRoom.SelectedRoomIndex = GetRandomRoomIndex(
                            NormalRoomClasses);

                    NewRoom.RoomType = ERoomType::Normal;

                    bPreviousWasMidBoss = false;

                    break;
                }
            }
        }

        NewRoom.ConnectedRoomIDs.Add(RoomID - 1);

        if (RoomID < DungeonRoomCount)
        {
            NewRoom.ConnectedRoomIDs.Add(RoomID + 1);
        }

        RoomNodes.Add(NewRoom);
    }

    CurrentRoomID = 0;

    SpawnCurrentRoom();
}

ERoomType AUT1_RoomManager::GetRandomRoomType(int32 RoomID,
    bool& bPreviousWasMidBoss) const
{
    const bool bCanSpawnMidBoss = RoomID >= MidBossMinRoom &&
        RoomID <= MidBossMaxRoom && !bPreviousWasMidBoss;

    const float NormalWeight = FMath::Max(
            0.0f, NormalRoomWeight);

    const float GimmickWeight = FMath::Max(
            0.0f, GimmickRoomWeight);

    const float MidBossWeight = bCanSpawnMidBoss
        ? FMath::Max(0.0f,MidBossRoomWeight) : 0.0f;


    const float TotalWeight =NormalWeight +GimmickWeight +
        MidBossWeight;

    if (TotalWeight <= 0.0f)
    {
        bPreviousWasMidBoss = false;
        return ERoomType::Normal;
    }

    const float RandomValue = FMath::FRandRange(
            0.0f, TotalWeight);

    if (RandomValue < NormalWeight)
    {
        bPreviousWasMidBoss = false;
        return ERoomType::Normal;
    }

    if (RandomValue < NormalWeight + GimmickWeight)
    {
        bPreviousWasMidBoss = false;

        return ERoomType::Gimmick;
    }

    if (bCanSpawnMidBoss)
    {
        bPreviousWasMidBoss = true;

        return ERoomType::MidBoss;
    }

    bPreviousWasMidBoss = false;

    return ERoomType::Normal;
}

int32 AUT1_RoomManager::GetRandomRoomIndex(
    const TArray<TSubclassOf<AUT1_RoomBase>>& RoomClasses) const
{
    if (RoomClasses.Num() == 0)
    {
        return INDEX_NONE;
    }

    return FMath::RandRange(0, RoomClasses.Num() - 1);
}

void AUT1_RoomManager::MoveToRoom(
    int32 NextRoomID)
{
    if (!CanMoveToRoom(NextRoomID))
    {
        return;
    }

    if (!RoomNodes.IsValidIndex(NextRoomID))
    {
        return;
    }

    ClosePortalWidget();
    DestroyCurrentPortal();

    if (CurrentRoomActor)
    {
        CurrentRoomActor->ResetRoom();
        CurrentRoomActor->Destroy();
        CurrentRoomActor = nullptr;
    }

    CurrentRoomID = NextRoomID;

    SpawnCurrentRoom();
}

void AUT1_RoomManager::MoveToNextRoom()
{
    if (CurrentRoomID == INDEX_NONE)
    {
        return;
    }

    const int32 NextRoomID =
        CurrentRoomID + 1;

    if (!RoomNodes.IsValidIndex(NextRoomID))
    {
        return;
    }

    MoveToRoom(NextRoomID);
}

void AUT1_RoomManager::MoveToBaseRoom()
{
    if (CurrentRoomID == 0)
    {
        return;
    }

    MoveToRoom(0);
}

bool AUT1_RoomManager::IsLastRoom() const
{
    return
        CurrentRoomID ==
        RoomNodes.Num() - 1;
}

void AUT1_RoomManager::SpawnCurrentRoom()
{
    if (!GetWorld())
    {
        return;
    }

    FRoomNode* RoomNode =
        FindRoomNode(CurrentRoomID);

    if (!RoomNode)
    {
        return;
    }

    TSubclassOf<AUT1_RoomBase> RoomClass =
        GetRoomClass(*RoomNode);

    if (!RoomClass)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;

    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::
        AlwaysSpawn;


    CurrentRoomActor =
        GetWorld()->SpawnActor<AUT1_RoomBase>(
            RoomClass,
            RoomSpawnLocation,
            FRotator::ZeroRotator,
            SpawnParams
        );


    if (!CurrentRoomActor)
    {
        return;
    }

    // 1번 방부터 RoomsPerEnemyLevel개마다 적 레벨 +1
    CurrentRoomActor->EnemyLevel =
        1 + FMath::Max(0, RoomNode->RoomID - 1) / FMath::Max(1, RoomsPerEnemyLevel);

    CurrentRoomActor->SetupRoom();

    APawn* PlayerPawn =
        UGameplayStatics::GetPlayerPawn(
            GetWorld(),
            0
        );

    if (PlayerPawn)
    {
        PlayerPawn->SetActorLocation(
            CurrentRoomActor->
            GetPlayerSpawnLocation()
        );
    }

    if (RoomNode->RoomType == ERoomType::Base)
    {
        SpawnCurrentPortal();
    }
    else if (RoomNode->RoomType == ERoomType::MidBoss &&
        RoomNode->bIsCleared)
    {
        SpawnCurrentPortal();
    }
    SpawnCurrentPortal();
}

TSubclassOf<AUT1_RoomBase> AUT1_RoomManager::GetRoomClass(
    FRoomNode& RoomNode)
{
    switch (RoomNode.RoomType)
    {
    case ERoomType::Base:
    {
        return BaseRoomClass;
    }
    case ERoomType::Normal:
    {
        if (!NormalRoomClasses.IsValidIndex(
            RoomNode.SelectedRoomIndex))
        {
            return nullptr;
        }

        return NormalRoomClasses[
            RoomNode.SelectedRoomIndex
        ];
    }
    case ERoomType::Gimmick:
    {
        if (!GimmickRoomClasses.IsValidIndex(
            RoomNode.SelectedRoomIndex))
        {
            return nullptr;
        }

        return GimmickRoomClasses[
            RoomNode.SelectedRoomIndex
        ];
    }

    case ERoomType::MidBoss:
    {
        if (!MidBossRoomClasses.IsValidIndex(
            RoomNode.SelectedRoomIndex))
        {
            return nullptr;
        }

        return MidBossRoomClasses[
            RoomNode.SelectedRoomIndex
        ];
    }

    case ERoomType::Boss:
    {
        if (!BossRoomClasses.IsValidIndex(
            RoomNode.SelectedRoomIndex))
        {
            return nullptr;
        }

        return BossRoomClasses[
            RoomNode.SelectedRoomIndex
        ];
    }


    default:
    {
        return nullptr;
    }
    }
}


FRoomNode* AUT1_RoomManager::FindRoomNode(
    int32 RoomID
)
{
    return RoomNodes.FindByPredicate(
        [RoomID](const FRoomNode& Node)
        {
            return Node.RoomID == RoomID;
        }
    );
}


const FRoomNode* AUT1_RoomManager::FindRoomNode(
    int32 RoomID
) const
{
    return RoomNodes.FindByPredicate(
        [RoomID](const FRoomNode& Node)
        {
            return Node.RoomID == RoomID;
        }
    );
}

bool AUT1_RoomManager::CanMoveToRoom(int32 RoomID) const
{
    const FRoomNode* CurrentRoom =
        FindRoomNode(CurrentRoomID);

    const FRoomNode* TargetRoom =
        FindRoomNode(RoomID);

    if (!CurrentRoom || !TargetRoom)
    {
        return false;
    }

    if (RoomID == CurrentRoomID)
    {
        return false;
    }

    if (RoomID == CurrentRoomID + 1)
    {
        return true;
    }

    if (TargetRoom->RoomType == ERoomType::Base)
    {
        return TargetRoom->bIsCleared;
    }

    if (TargetRoom->RoomType == ERoomType::MidBoss)
    {
        return TargetRoom->bIsCleared;
    }


    return false;
}

void AUT1_RoomManager::MarkCurrentRoomCleared()
{
    FRoomNode* CurrentRoom =
        FindRoomNode(CurrentRoomID);

    if (!CurrentRoom)
    {
        return;
    }

    CurrentRoom->bIsCleared = true;

    if (CurrentRoom->RoomType == ERoomType::MidBoss)
    {
        SpawnCurrentPortal();
    }
}

const FRoomNode* AUT1_RoomManager::GetCurrentRoomNode() const
{
    return FindRoomNode(CurrentRoomID);
}

int32 AUT1_RoomManager::GetConnectedRoomID(
    int32 Direction) const
{
    const FRoomNode* CurrentNode =
        GetCurrentRoomNode();

    if (!CurrentNode)
    {
        return INDEX_NONE;
    }

    if (Direction == 0)
    {
        if (CurrentNode->ConnectedRoomIDs.Num() > 0)
        {
            return CurrentNode->
                ConnectedRoomIDs[0];
        }
    }


    if (Direction == 1)
    {
        if (CurrentNode->ConnectedRoomIDs.Num() > 1)
        {
            return CurrentNode->
                ConnectedRoomIDs[1];
        }
    }


    return INDEX_NONE;
}

void AUT1_RoomManager::SpawnCurrentPortal()
{
    if (!GetWorld())
    {
        return;
    }

    if (!PortalClass)
    {
        return;
    }


    DestroyCurrentPortal();


    FActorSpawnParameters SpawnParams;

    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::
        AlwaysSpawn;


    CurrentPortal =
        GetWorld()->SpawnActor<AUT1_Portal>(
            PortalClass,
            PortalSpawnLocation,
            FRotator::ZeroRotator,
            SpawnParams
        );


    if (!CurrentPortal)
    {
        return;
    }
}

void AUT1_RoomManager::DestroyCurrentPortal()
{
    if (CurrentPortal)
    {
        CurrentPortal->Destroy();

        CurrentPortal = nullptr;
    }
}

void AUT1_RoomManager::TryInteractPortal()
{
    if (!CurrentPortal)
    {
        return;
    }

    if (PortalWidget)
    {
        ClosePortalWidget();
        return;
    }

    if (!CurrentPortal->IsPlayerInRange())
    {
        return;
    }

    if (!PortalWidgetClass)
    {
        return;
    }

    PortalWidget =
        CreateWidget<UUT1_PortalWidget>(
            GetWorld(),
            PortalWidgetClass
        );

    PortalWidget->AddToViewport();

    PortalWidget->SetVisibility(
        ESlateVisibility::Visible);
}

void AUT1_RoomManager::ClosePortalWidget()
{
    if (!PortalWidget)
    {
        return;
    }

    PortalWidget->RemoveFromParent();

    PortalWidget = nullptr;
}

void AUT1_RoomManager::TestMoveToBaseRoom()
{
    MoveToBaseRoom();
}
