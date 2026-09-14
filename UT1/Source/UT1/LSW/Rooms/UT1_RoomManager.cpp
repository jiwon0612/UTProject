// Fill out your copyright notice in the Description page of Project Settings.


#include "LSW/Rooms/UT1_RoomManager.h"
#include "LSW/Rooms/UT1_RoomBase.h"
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

    GenerateDungeon();

    EnableInput(
        GetWorld()->GetFirstPlayerController()
    );

    if (!InputComponent)
    {
        return;
    }

    // P = Base로 이동
    InputComponent->BindKey(
        EKeys::P,
        IE_Pressed,
        this,
        &AUT1_RoomManager::TestMoveToBaseRoom
    );

    // 1~6 = 육각형 방향 이동
    InputComponent->BindKey(
        EKeys::One,
        IE_Pressed,
        this,
        &AUT1_RoomManager::MoveDirection1
    );

    InputComponent->BindKey(
        EKeys::Two,
        IE_Pressed,
        this,
        &AUT1_RoomManager::MoveDirection2
    );

    InputComponent->BindKey(
        EKeys::Three,
        IE_Pressed,
        this,
        &AUT1_RoomManager::MoveDirection3
    );

    InputComponent->BindKey(
        EKeys::Four,
        IE_Pressed,
        this,
        &AUT1_RoomManager::MoveDirection4
    );

    InputComponent->BindKey(
        EKeys::Five,
        IE_Pressed,
        this,
        &AUT1_RoomManager::MoveDirection5
    );

    InputComponent->BindKey(
        EKeys::Six,
        IE_Pressed,
        this,
        &AUT1_RoomManager::MoveDirection6
    );
}

void AUT1_RoomManager::GenerateDungeon()
{
    RoomNodes.Empty();

    // Base 생성
    FRoomNode BaseNode;

    BaseNode.RoomID = 0;
    BaseNode.RoomType = ERoomType::Base;

    BaseNode.HexQ = 0;
    BaseNode.HexR = 0;

    RoomNodes.Add(BaseNode);

    CreateRandomHexDungeon();

    CurrentRoomID = 0;

    SpawnCurrentRoom();
}


void AUT1_RoomManager::MoveToRoom(int32 NextRoomID)
{
    FRoomNode* NextRoomNode =
        FindRoomNode(NextRoomID);

    FRoomNode* CurrentRoomNode =
        FindRoomNode(CurrentRoomID);

    if (!NextRoomNode || !CurrentRoomNode)
    {
        return;
    }

    if (CurrentRoomActor)
    {
        CurrentRoomActor->Destroy();
        CurrentRoomActor = nullptr;
    }


    CurrentRoomID = NextRoomID;

    SpawnCurrentRoom();
}

void AUT1_RoomManager::CreateRandomHexDungeon()
{
    // Axial Coordinate 기준 6방향
    const TArray<FIntPoint> Directions =
    {
        FIntPoint(0, -1),
        FIntPoint(1, -1),
        FIntPoint(1, 0),
        FIntPoint(0, 1),
        FIntPoint(-1, 1),
        FIntPoint(-1, 0)
    };


    int32 AttemptCount = 0;

    const int32 MaxAttempts =
        DungeonRoomCount * 100;

    while (RoomNodes.Num() < DungeonRoomCount &&
        AttemptCount < MaxAttempts)
    {
        AttemptCount++;

        // 이미 존재하는 방 중 하나 선택
        int32 RandomRoomIndex =
            FMath::RandRange(
                0,
                RoomNodes.Num() - 1
            );


        FRoomNode& ParentRoom =
            RoomNodes[RandomRoomIndex];


        // 랜덤 방향 선택
        int32 RandomDirectionIndex =
            FMath::RandRange(0, 5);


        FIntPoint Direction =
            Directions[RandomDirectionIndex];


        int32 NewQ =
            ParentRoom.HexQ + Direction.X;

        int32 NewR =
            ParentRoom.HexR + Direction.Y;


        // 해당 위치에 이미 방이 있으면 다시 시도
        if (
            GetRoomIDByHexCoordinate(
                NewQ,
                NewR
            )
            != INDEX_NONE
            )
        {
            continue;
        }

        FRoomNode NewRoom;

        NewRoom.RoomID = RoomNodes.Num();

        NewRoom.RoomType = ERoomType::Normal;

        NewRoom.HexQ = NewQ;
        NewRoom.HexR = NewR;


        // 방 생성 시점에 실제 방 Blueprint 랜덤 결정
        if (NormalRoomClasses.Num() > 0)
        {
            NewRoom.SelectedRoomIndex =
                FMath::RandRange(
                    0,
                    NormalRoomClasses.Num() - 1
                );
        }
        else
        {
            NewRoom.SelectedRoomIndex =
                INDEX_NONE;

            UE_LOG(
                LogTemp,
                Error,
                TEXT("NormalRoomClasses가 비어있습니다.")
            );
        }


        RoomNodes.Add(NewRoom);
    }


    // 생성 완료 후 연결 관계 계산
    for (FRoomNode& RoomNode : RoomNodes)
    {
        RoomNode.ConnectedRoomIDs.Empty();

        for (const FIntPoint& Direction : Directions)
        {
            int32 NeighborQ =
                RoomNode.HexQ + Direction.X;

            int32 NeighborR =
                RoomNode.HexR + Direction.Y;


            int32 NeighborID =
                GetRoomIDByHexCoordinate(
                    NeighborQ,
                    NeighborR
                );


            if (NeighborID != INDEX_NONE)
            {
                RoomNode
                    .ConnectedRoomIDs
                    .Add(NeighborID);
            }
        }
    }

    if (RoomNodes.Num() < DungeonRoomCount)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "요청한 방 개수를 모두 생성하지 못했습니다. 생성됨: %d / %d"
            ),
            RoomNodes.Num(),
            DungeonRoomCount
        );
    }
}

int32 AUT1_RoomManager::GetRoomIDByHexCoordinate(
    int32 HexQ,
    int32 HexR
) const
{
    for (const FRoomNode& RoomNode : RoomNodes)
    {
        if (
            RoomNode.HexQ == HexQ &&
            RoomNode.HexR == HexR
            )
        {
            return RoomNode.RoomID;
        }
    }

    return INDEX_NONE;
}

void AUT1_RoomManager::MoveDirection1()
{
    TestMoveHexDirection(1);
}

void AUT1_RoomManager::MoveDirection2()
{
    TestMoveHexDirection(2);
}

void AUT1_RoomManager::MoveDirection3()
{
    TestMoveHexDirection(3);
}

void AUT1_RoomManager::MoveDirection4()
{
    TestMoveHexDirection(4);
}

void AUT1_RoomManager::MoveDirection5()
{
    TestMoveHexDirection(5);
}

void AUT1_RoomManager::MoveDirection6()
{
    TestMoveHexDirection(6);
}

void AUT1_RoomManager::SpawnCurrentRoom()
{
    // 현재 RoomID의 논리적 방 데이터 찾기
    FRoomNode* RoomNode =
        FindRoomNode(CurrentRoomID);

    if (!RoomNode)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("RoomID %d를 찾을 수 없습니다."),
            CurrentRoomID
        );

        return;
    }


    // RoomNode에 맞는 실제 방 Class 가져오기
    TSubclassOf<AUT1_RoomBase> RoomClass =
        GetRoomClass(*RoomNode);

    if (!RoomClass)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("RoomClass가 없습니다.")
        );

        return;
    }


    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Spawn할 RoomClass : %s"),
        *RoomClass->GetName()
    );


    // ==============================
    // 실제 방 생성
    // ==============================

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


    // Spawn 실패
    if (!CurrentRoomActor)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Room Spawn Failed")
        );

        return;
    }


    // Spawn 성공
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Room Spawn Success")
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Room Location : %s"),
        *CurrentRoomActor
        ->GetActorLocation()
        .ToString()
    );


    // 방 초기화
    CurrentRoomActor->SetupRoom();


    // ==============================
    // 플레이어 위치 이동
    // ==============================

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


    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Player Location : %s"),
        *PlayerSpawnLocation.ToString()
    );
}


FRoomNode* AUT1_RoomManager::FindRoomNode(int32 RoomID
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


const FRoomNode* AUT1_RoomManager::FindRoomNode(int32 RoomID) const
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


TSubclassOf<AUT1_RoomBase>AUT1_RoomManager::GetRoomClass(
    FRoomNode& RoomNode
)
{
    switch (RoomNode.RoomType)
    {
        // ==============================
        // Base
        // ==============================

    case ERoomType::Base:
    {
        return BaseRoomClass;
    }


    // ==============================
    // Normal
    // ==============================

    case ERoomType::Normal:
    {
        if (NormalRoomClasses.Num() == 0)
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT(
                    "NormalRoomClasses가 비어있습니다."
                )
            );

            return nullptr;
        }


        /*
         * 아직 이 RoomNode의 실제 방이
         * 결정되지 않았다면 랜덤 선택
         */
        if (RoomNode.SelectedRoomIndex == INDEX_NONE)
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT(
                    "Room %d의 실제 방이 결정되지 않았습니다."
                ),
                RoomNode.RoomID
            );

            return nullptr;
        }

        /*
         * 저장된 Index가 현재 배열에서도
         * 유효한지 확인
         */
        if (!NormalRoomClasses.IsValidIndex(
                RoomNode.SelectedRoomIndex))
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT(
                    "잘못된 Room Index입니다."
                )
            );

            return nullptr;
        }


        return NormalRoomClasses[
            RoomNode.SelectedRoomIndex];
    }


    // ==============================
    // 아직 구현하지 않은 방
    // ==============================

    default:
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "아직 구현되지 않은 RoomType입니다."
            )
        );

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
                TEXT("P 입력 - Base Room %d로 이동"),
                RoomNode.RoomID
            );

            MoveToRoom(RoomNode.RoomID);

            return;
        }
    }

    UE_LOG(
        LogTemp,
        Error,
        TEXT("Base Room을 찾을 수 없습니다.")
    );
}

void AUT1_RoomManager::TestMoveHexDirection(
    int32 Direction
)
{
    FRoomNode* CurrentNode =
        FindRoomNode(CurrentRoomID);

    if (!CurrentNode)
    {
        return;
    }

    int32 DirectionQ = 0;
    int32 DirectionR = 0;

    switch (Direction)
    {
    case 1:
        DirectionQ = 0;
        DirectionR = -1;
        break;

    case 2:
        DirectionQ = 1;
        DirectionR = -1;
        break;

    case 3:
        DirectionQ = 1;
        DirectionR = 0;
        break;

    case 4:
        DirectionQ = 0;
        DirectionR = 1;
        break;

    case 5:
        DirectionQ = -1;
        DirectionR = 1;
        break;

    case 6:
        DirectionQ = -1;
        DirectionR = 0;
        break;

    default:
        return;
    }

    int32 TargetQ =
        CurrentNode->HexQ + DirectionQ;

    int32 TargetR =
        CurrentNode->HexR + DirectionR;

    FRoomNode* TargetNode =
        FindRoomByHexCoordinate(
            TargetQ,
            TargetR
        );

    if (!TargetNode)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Direction %d 방향에는 방이 없습니다."
            ),
            Direction
        );

        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "Direction %d : Room %d -> Room %d"
        ),
        Direction,
        CurrentRoomID,
        TargetNode->RoomID
    );

    MoveToRoom(
        TargetNode->RoomID
    );
}

FRoomNode*
AUT1_RoomManager::FindRoomByHexCoordinate(
    int32 HexQ,
    int32 HexR
)
{
    for (FRoomNode& RoomNode : RoomNodes)
    {
        if (
            RoomNode.HexQ == HexQ &&
            RoomNode.HexR == HexR
            )
        {
            return &RoomNode;
        }
    }

    return nullptr;
}

const FRoomNode* AUT1_RoomManager::FindRoomByHexCoordinate(
    int32 HexQ,
    int32 HexR
) const
{
    for (const FRoomNode& RoomNode : RoomNodes)
    {
        if (
            RoomNode.HexQ == HexQ &&
            RoomNode.HexR == HexR
            )
        {
            return &RoomNode;
        }
    }

    return nullptr;
}