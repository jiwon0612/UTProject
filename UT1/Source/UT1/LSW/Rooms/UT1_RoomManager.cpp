#include "LSW/Rooms/UT1_RoomManager.h"
#include "LSW/Rooms/UT1_RoomBase.h"
#include "LSW/Rooms/UT1_Portal.h"
#include "LSW/Widget/UT1_PortalWidget.h"
#include "LSW/Widget/UT1_RoomTransitionWidget.h"
#include "LSW/Widget/UT1_GameClearWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "UObject/ConstructorHelpers.h"

AUT1_RoomManager::AUT1_RoomManager()
{
    PrimaryActorTick.bCanEverTick = false;

	// Keep the room flow usable from a plain C++ RoomManager too; Blueprint
	// instances can still replace either class in their defaults.
	static ConstructorHelpers::FClassFinder<AUT1_Portal> PortalBlueprint(
		TEXT("/Game/LSW/BP/Rooms/BP_Portal"));
	if (PortalBlueprint.Succeeded())
	{
		PortalClass = PortalBlueprint.Class;
	}

	static ConstructorHelpers::FClassFinder<UUT1_PortalWidget> PortalWidgetBlueprint(
		TEXT("/Game/LSW/BP/Widget/BP_PortalWidget"));
	if (PortalWidgetBlueprint.Succeeded())
	{
		PortalWidgetClass = PortalWidgetBlueprint.Class;
	}
}

void AUT1_RoomManager::BeginPlay()
{
    Super::BeginPlay();

	// Recover even if an existing Blueprint default explicitly cleared these classes.
	if (!PortalClass)
	{
		PortalClass = LoadClass<AUT1_Portal>(nullptr,
			TEXT("/Game/LSW/BP/Rooms/BP_Portal.BP_Portal_C"));
	}
	if (!PortalWidgetClass)
	{
		PortalWidgetClass = LoadClass<UUT1_PortalWidget>(nullptr,
			TEXT("/Game/LSW/BP/Widget/BP_PortalWidget.BP_PortalWidget_C"));
	}

    GenerateDungeon();
}

void AUT1_RoomManager::GenerateDungeon()
{
    RoomNodes.Empty();
    bMidBossClearedThisRun = false;

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

                case ERoomType::Reward:
                {
                    NewRoom.SelectedRoomIndex = GetRandomRoomIndex(RewardRoomClasses);

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

    const float RewardWeight = FMath::Max(
            0.0f, RewardRoomWeight);

    const float MidBossWeight = bCanSpawnMidBoss
        ? FMath::Max(0.0f,MidBossRoomWeight) : 0.0f;


    const float TotalWeight = NormalWeight + RewardWeight +
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

    if (RandomValue < NormalWeight + RewardWeight)
    {
        bPreviousWasMidBoss = false;

        return ERoomType::Reward;
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
	if (bRoomTransitionInProgress)
	{
		return;
	}

    if (!CanMoveToRoom(NextRoomID))
    {
        return;
    }

    if (!RoomNodes.IsValidIndex(NextRoomID))
    {
        return;
    }

    APlayerController* PlayerController = GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("[RoomTransition] Cannot move rooms: no player controller."));
		return;
	}

	if (!RoomTransitionWidgetClass)
	{
		RoomTransitionWidgetClass = UUT1_RoomTransitionWidget::StaticClass();
	}

	RoomTransitionWidget = CreateWidget<UUT1_RoomTransitionWidget>(PlayerController, RoomTransitionWidgetClass);
	if (!RoomTransitionWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[RoomTransition] Failed to create transition widget class %s."),
			*GetNameSafe(RoomTransitionWidgetClass.Get()));
		return;
	}

	ClosePortalWidget();
    DestroyCurrentPortal();
	PendingRoomID = NextRoomID;
	bRoomTransitionInProgress = true;
	RoomTransitionWidget->OnFadeOutFinished.AddDynamic(this, &AUT1_RoomManager::CompletePendingRoomMove);
	RoomTransitionWidget->OnFadeInFinished.AddDynamic(this, &AUT1_RoomManager::FinishRoomTransition);
	RoomTransitionWidget->AddToViewport(10000);
	UE_LOG(LogTemp, Log, TEXT("[RoomTransition] Widget added to viewport for room %d -> %d."), CurrentRoomID, NextRoomID);
	RoomTransitionWidget->PlayFadeOut();
}

void AUT1_RoomManager::CompletePendingRoomMove()
{
	if (!bRoomTransitionInProgress || PendingRoomID == INDEX_NONE || !RoomNodes.IsValidIndex(PendingRoomID))
	{
		if (RoomTransitionWidget)
		{
			RoomTransitionWidget->PlayFadeIn();
		}
		return;
	}

    if (CurrentRoomActor)
    {
        CurrentRoomActor->ResetRoom();
        CurrentRoomActor->Destroy();
        CurrentRoomActor = nullptr;
    }

    CurrentRoomID = PendingRoomID;
    PendingRoomID = INDEX_NONE;

    SpawnCurrentRoom();
	RoomTransitionWidget->PlayFadeIn();
}

void AUT1_RoomManager::FinishRoomTransition()
{
	if (RoomTransitionWidget)
	{
		RoomTransitionWidget->RemoveFromParent();
		RoomTransitionWidget = nullptr;
	}
	PendingRoomID = INDEX_NONE;
	bRoomTransitionInProgress = false;
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
	CurrentRoomActor->EnemyDifficultyMultiplier =
		(RoomNode->RoomType == ERoomType::Normal && RoomNode->bIsCleared)
		? 0.75f
		: 1.0f;

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

    if (RoomNode->RoomType == ERoomType::Base ||
        ((RoomNode->RoomType == ERoomType::MidBoss ||
          RoomNode->RoomType == ERoomType::Reward) && RoomNode->bIsCleared))
    {
        SpawnCurrentPortal();
    }
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
    case ERoomType::Reward:
    {
        if (!RewardRoomClasses.IsValidIndex(
            RoomNode.SelectedRoomIndex))
        {
            return nullptr;
        }

        return RewardRoomClasses[
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

    // 중간보스를 쓰러뜨리면 이번 런 동안 발견한 모든 방으로 이동할 수 있다.
    if (bMidBossClearedThisRun)
    {
        return true;
    }

    if (RoomID == CurrentRoomID + 1)
    {
        return true;
    }

	// Previously cleared normal rooms can be revisited; their SetupRoom respawn
	// path applies the reduced difficulty multiplier in SpawnCurrentRoom().
	if (TargetRoom->RoomType == ERoomType::Normal && TargetRoom->bIsCleared)
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
        bMidBossClearedThisRun = true;
    }

    if (CurrentRoom->RoomType != ERoomType::Base)
    {
        SpawnCurrentPortal();
    }

	UE_LOG(LogTemp, Log, TEXT("[Room] Room %d cleared (type=%d)."),
		CurrentRoom->RoomID, static_cast<int32>(CurrentRoom->RoomType));
}

void AUT1_RoomManager::ShowGameClear()
{
    if (!GetWorld() || IsValid(GameClearWidget))
    {
        return;
    }

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameClear] Cannot show screen: no player controller."));
        return;
    }

    GameClearWidget = CreateWidget<UUT1_GameClearWidget>(PlayerController,
        UUT1_GameClearWidget::StaticClass());
    if (GameClearWidget)
    {
        GameClearWidget->Show();
        UGameplayStatics::SetGamePaused(GetWorld(), true);
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
		UE_LOG(LogTemp, Error, TEXT("[Room] Cannot spawn portal: PortalClass is not assigned."));
        return;
    }

	if (IsValid(CurrentPortal))
	{
		return;
	}


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
		UE_LOG(LogTemp, Error, TEXT("[Room] Portal spawn failed for room %d."), CurrentRoomID);
        return;
    }

	UE_LOG(LogTemp, Log, TEXT("[Room] Portal spawned for room %d at %s."),
		CurrentRoomID, *PortalSpawnLocation.ToString());
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
