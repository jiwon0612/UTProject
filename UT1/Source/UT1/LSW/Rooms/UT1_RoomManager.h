#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UT1_RoomManager.generated.h"

class AUT1_RoomBase;
class AUT1_Portal;
class UUserWidget;
class UUT1_PortalWidget;

struct FRoomNode;
enum class ERoomType : uint8;

UCLASS()
class UT1_API AUT1_RoomManager : public AActor
{
    GENERATED_BODY()

public:
    AUT1_RoomManager();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable)
    void GenerateDungeon();

    UFUNCTION(BlueprintCallable)
    void MoveToRoom(int32 NextRoomID);

    UFUNCTION(BlueprintCallable)
    void MoveToNextRoom();

    UFUNCTION(BlueprintCallable)
    void MoveToBaseRoom();

    UFUNCTION(BlueprintPure)
    bool IsLastRoom() const;

    void SpawnCurrentRoom();

    TSubclassOf<AUT1_RoomBase> GetRoomClass(FRoomNode& RoomNode);

    const FRoomNode* GetCurrentRoomNode() const;

    int32 GetConnectedRoomID(int32 Direction) const;

    void SpawnCurrentPortal();

    void DestroyCurrentPortal();

    void TryInteractPortal();

    void ClosePortalWidget();

    const TArray<FRoomNode>& GetRoomNodes() const
    {
        return RoomNodes;
    }

    int32 GetCurrentRoomID() const
    {
        return CurrentRoomID;
    }

    const FRoomNode* FindRoomNode(int32 RoomID) const;

    bool CanMoveToRoom(int32 RoomID) const;

    UFUNCTION(BlueprintCallable)
    void MarkCurrentRoomCleared();

    FRoomNode* FindRoomNode(int32 RoomID);

private:
    ERoomType GetRandomRoomType(int32 RoomID, bool& bPreviousWasMidBoss) const;

    int32 GetRandomRoomIndex(const TArray<TSubclassOf<AUT1_RoomBase>>& RoomClasses) const;

    UFUNCTION()
    void TestMoveToBaseRoom();

public:
    UPROPERTY(EditAnywhere, Category = "Room")
    TSubclassOf<AUT1_RoomBase> BaseRoomClass;

    UPROPERTY(EditAnywhere, Category = "Room")
    TArray<TSubclassOf<AUT1_RoomBase>> NormalRoomClasses;

    // Gimmick = 보상/특수 방 역할
    UPROPERTY(EditAnywhere, Category = "Room")
    TArray<TSubclassOf<AUT1_RoomBase>> GimmickRoomClasses;

    UPROPERTY(EditAnywhere, Category = "Room")
    TArray<TSubclassOf<AUT1_RoomBase>> MidBossRoomClasses;

    UPROPERTY(EditAnywhere, Category = "Room")
    TArray<TSubclassOf<AUT1_RoomBase>> BossRoomClasses;

    UPROPERTY(EditAnywhere, Category = "Dungeon")
    int32 DungeonRoomCount = 30;

    // 이 수만큼 방을 지날 때마다 적 레벨이 1 오른다 (1~3번 방 Lv1, 4~6번 방 Lv2 ...)
    UPROPERTY(EditAnywhere, Category = "Dungeon", meta = (ClampMin = "1"))
    int32 RoomsPerEnemyLevel = 3;

    UPROPERTY(EditAnywhere, Category = "Dungeon|Probability")
    float NormalRoomWeight = 60.0f;

    UPROPERTY(EditAnywhere, Category = "Dungeon|Probability")
    float GimmickRoomWeight = 25.0f;

    UPROPERTY(EditAnywhere, Category = "Dungeon|Probability")
    float MidBossRoomWeight = 15.0f;

    UPROPERTY(EditAnywhere, Category = "Dungeon|MidBoss")
    int32 MidBossMinRoom = 5;

    UPROPERTY(EditAnywhere, Category = "Dungeon|MidBoss")
    int32 MidBossMaxRoom = 25;

    UPROPERTY()
    TArray<FRoomNode> RoomNodes;

    UPROPERTY(BlueprintReadOnly, Category = "Room")
    int32 CurrentRoomID = INDEX_NONE;

    UPROPERTY()
    AUT1_RoomBase* CurrentRoomActor = nullptr;

    UPROPERTY(EditAnywhere, Category = "Room")
    FVector RoomSpawnLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Portal")
    TSubclassOf<AUT1_Portal> PortalClass;

    UPROPERTY()
    AUT1_Portal* CurrentPortal = nullptr;

    UPROPERTY(EditAnywhere, Category = "Portal")
    FVector PortalSpawnLocation =
        FVector(1000.f, 0.f, 100.f);

    UPROPERTY(EditAnywhere, Category = "Portal|Widget")
    TSubclassOf<UUT1_PortalWidget> PortalWidgetClass;

    UPROPERTY()
    UUT1_PortalWidget* PortalWidget = nullptr;
};