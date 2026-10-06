#include "LSW/Rooms/UT1_RoomManager.h"
#include "LSW/Rooms/UT1_RoomBase.h"
#include "LSW/Rooms/UT1_Portal.h"
#include "Blueprint/UserWidget.h"
#include "LSW/Widget/UT1_PortalWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"


AUT1_RoomManager::AUT1_RoomManager()
{
    PrimaryActorTick.bCanEverTick = false;
}


void AUT1_RoomManager::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC =
        GetWorld()->GetFirstPlayerController();

    if (PC)
    {
        EnableInput(PC);
    }

    if (InputComponent)
    {
        InputComponent->BindKey(
            EKeys::E,
            IE_Pressed,
            this,
            &AUT1_RoomManager::TryInteractPortal
        );
    }
    else
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[RoomManager] InputComponent is NULL")
        );
    }

    GenerateDungeon();
}

void AUT1_RoomManager::GenerateDungeon()
{
    RoomNodes.Empty();

    // =========================
    // Base
    // =========================

    FRoomNode BaseRoom;

    BaseRoom.RoomID = 0;
    BaseRoom.RoomType = ERoomType::Base;

    RoomNodes.Add(BaseRoom);


    // =========================
    // Normal Rooms
    // =========================

    const int32 RoomCount = NormalRoomClasses.Num();

    for (int32 i = 0; i < RoomCount; ++i)
    {
        FRoomNode NewRoom;

        NewRoom.RoomID = i + 1;
        NewRoom.RoomType = ERoomType::Normal;

        // 이전 방과 다음 방 연결
        if (i == 0)
        {
            NewRoom.ConnectedRoomIDs.Add(0);
        }
        else
        {
            NewRoom.ConnectedRoomIDs.Add(i);
        }

        if (i + 1 < RoomCount)
        {
            NewRoom.ConnectedRoomIDs.Add(i + 2);
        }

        // 방 Blueprint를 생성 시점에 랜덤 결정
        NewRoom.SelectedRoomIndex =
            FMath::RandRange(
                0,
                NormalRoomClasses.Num() - 1
            );

        RoomNodes.Add(NewRoom);
    }


    // 시작
    CurrentRoomID = 0;

    SpawnCurrentRoom();
}

void AUT1_RoomManager::MoveToRoom(
    int32 NextRoomID)
{
    FRoomNode* NextRoomNode = FindRoomNode(NextRoomID);

    FRoomNode* CurrentRoomNode = FindRoomNode(CurrentRoomID);

    if (!NextRoomNode || !CurrentRoomNode)
    {
        return;
    }

    ClosePortalWidget();

    DestroyCurrentPortal();

    if (CurrentRoomActor)
    {
        CurrentRoomActor->Destroy();

        CurrentRoomActor = nullptr;
    }

    CurrentRoomID = NextRoomID;

    SpawnCurrentRoom();
}

void AUT1_RoomManager::MoveToNextRoom()
{
    const int32 NextRoomID =
        CurrentRoomID + 1;

    if (!RoomNodes.IsValidIndex(NextRoomID))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("더 이상 이동할 방이 없습니다.")
        );

        return;
    }

    MoveToRoom(NextRoomID);
}

void AUT1_RoomManager::MoveToBaseRoom()
{
    if (CurrentRoomID == 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("이미 Base에 있습니다.")
        );

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
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

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

    CurrentRoomActor->SetupRoom();

    APlayerController* PlayerController =
        GetWorld()->GetFirstPlayerController();

    if (!PlayerController)
    {
        return;
    }

    APawn* PlayerPawn =
        PlayerController->GetPawn();

    if (!PlayerPawn)
    {
        return;
    }

    FVector PlayerSpawnLocation =
        CurrentRoomActor->GetPlayerSpawnLocation();


    PlayerPawn->SetActorLocation(
        PlayerSpawnLocation
    );

    SpawnCurrentPortal();
}

void AUT1_RoomManager::SpawnCurrentPortal()
{
    if (!PortalClass)
    {
        return;
    }

    DestroyCurrentPortal();

    FActorSpawnParameters SpawnParams;

    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    CurrentPortal =
        GetWorld()->SpawnActor<AUT1_Portal>(
            PortalClass,
            PortalSpawnLocation,
            FRotator::ZeroRotator,
            SpawnParams
        );
}

void AUT1_RoomManager::DestroyCurrentPortal()
{
    if (IsValid(CurrentPortal))
    {
        CurrentPortal->Destroy();
    }

    CurrentPortal = nullptr;
}

void AUT1_RoomManager::TryInteractPortal()
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Portal] E Pressed")
    );

    if (!IsValid(CurrentPortal))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Portal] CurrentPortal INVALID")
        );

        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Portal] CurrentPortal VALID")
    );

    if (!CurrentPortal->IsPlayerInRange())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[Portal] Player is NOT in range")
        );

        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Portal] Player IN range")
    );

    if (!PortalWidgetClass)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Portal] PortalWidgetClass is NULL")
        );

        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Portal] PortalWidgetClass VALID")
    );

    if (IsValid(PortalWidget))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[Portal] PortalWidget already exists")
        );

        return;
    }

    APlayerController* PC =
        GetWorld()->GetFirstPlayerController();

    if (!PC)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Portal] PlayerController NULL")
        );

        return;
    }

    PortalWidget =
        CreateWidget<UUT1_PortalWidget>(
            PC,
            PortalWidgetClass
        );

    if (!PortalWidget)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Portal] CreateWidget FAILED")
        );

        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Portal] CreateWidget SUCCESS")
    );

    PortalWidget->AddToViewport(100);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Portal] AddToViewport SUCCESS")
    );
}

void AUT1_RoomManager::ClosePortalWidget()
{
    if (IsValid(PortalWidget))
    {
        PortalWidget->RemoveFromParent();
        PortalWidget = nullptr;
    }
}

const FRoomNode*
AUT1_RoomManager::GetCurrentRoomNode() const
{
    return FindRoomNode(CurrentRoomID);
}

FRoomNode*
AUT1_RoomManager::FindRoomNode(
    int32 RoomID
)
{
    for (FRoomNode& RoomNode : RoomNodes)
    {
        if (RoomNode.RoomID == RoomID)
        {
            return &RoomNode;
        }
    }

    return nullptr;
}


const FRoomNode*
AUT1_RoomManager::FindRoomNode(
    int32 RoomID
) const
{
    for (const FRoomNode& RoomNode : RoomNodes)
    {
        if (RoomNode.RoomID == RoomID)
        {
            return &RoomNode;
        }
    }

    return nullptr;
}

TSubclassOf<AUT1_RoomBase>
AUT1_RoomManager::GetRoomClass(FRoomNode& RoomNode)
{
    switch (RoomNode.RoomType)
    {
        case ERoomType::Base:
        {
            return BaseRoomClass;
        }


        case ERoomType::Normal:
        {
            if (NormalRoomClasses.Num() == 0)
            {
                return nullptr;
            }

            if (RoomNode.SelectedRoomIndex ==INDEX_NONE)
            {
                return nullptr;
            }


            if (!NormalRoomClasses.IsValidIndex(RoomNode.SelectedRoomIndex))
            {
                return nullptr;
            }
            return NormalRoomClasses[RoomNode.SelectedRoomIndex];
        }
        default:
        {
            return nullptr;
        }
    }
}

void AUT1_RoomManager::TestMoveToBaseRoom()
{
    for (const FRoomNode& RoomNode : RoomNodes)
    {
        if (RoomNode.RoomType == ERoomType::Base)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT(
                    "P 입력 - Base Room %d로 이동"
                ),
                RoomNode.RoomID
            );

            MoveToRoom(
                RoomNode.RoomID
            );

            return;
        }
    }
}