// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UT1_RoomManager.generated.h"

class AUT1_RoomBase;
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


    // 1~6 방향 이동
    UFUNCTION()
    void MoveDirection1();

    UFUNCTION()
    void MoveDirection2();

    UFUNCTION()
    void MoveDirection3();

    UFUNCTION()
    void MoveDirection4();

    UFUNCTION()
    void MoveDirection5();

    UFUNCTION()
    void MoveDirection6();

    void SpawnCurrentRoom();
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


private:

    // Room ID로 찾기
    FRoomNode* FindRoomNode(int32 RoomID);

    const FRoomNode* FindRoomNode(int32 RoomID) const;


    // Hex 좌표로 찾기
    FRoomNode* FindRoomByHexCoordinate(
        int32 HexQ,
        int32 HexR
    );

    const FRoomNode* FindRoomByHexCoordinate(
        int32 HexQ,
        int32 HexR
    ) const;

    void CreateRandomHexDungeon();

    bool TryCreateRoomAt(
        int32 HexQ,
        int32 HexR
    );

    int32 GetRoomIDByHexCoordinate(
        int32 HexQ,
        int32 HexR
    ) const;


    // RoomType에 맞는 실제 Blueprint Class 반환
    TSubclassOf<AUT1_RoomBase> GetRoomClass(
        FRoomNode& RoomNode
    );


private:
    UFUNCTION()
    void TestMoveToBaseRoom();
    void TestMoveHexDirection(int32 Direction);
};
