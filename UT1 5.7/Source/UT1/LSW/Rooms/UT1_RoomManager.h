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

    // Base로 복귀
    UFUNCTION(BlueprintCallable)
    void MoveToBaseRoom();

    // 현재 방이 마지막 방인지
    UFUNCTION(BlueprintPure)
    bool IsLastRoom() const;

    void SpawnCurrentRoom();

    void SpawnCurrentPortal();

    void DestroyCurrentPortal();

    void TryInteractPortal();

    void ClosePortalWidget();

    const FRoomNode* GetCurrentRoomNode() const;

    int32 GetConnectedRoomID(
        int32 Direction
    ) const;


public:

    UPROPERTY(EditAnywhere, Category = "Room")
    TSubclassOf<AUT1_RoomBase> BaseRoomClass;

    UPROPERTY(EditAnywhere, Category = "Room")
    TArray<TSubclassOf<AUT1_RoomBase>> NormalRoomClasses;

    UPROPERTY(EditAnywhere, Category = "Dungeon")
    int32 DungeonRoomCount = 30;

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


private:

    FRoomNode* FindRoomNode(
        int32 RoomID
    );

    const FRoomNode* FindRoomNode(
        int32 RoomID
    ) const;

    TSubclassOf<AUT1_RoomBase> GetRoomClass(
        FRoomNode& RoomNode
    );


private:

    UFUNCTION()
    void TestMoveToBaseRoom();
};