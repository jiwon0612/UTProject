// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UT1_RoomBase.generated.h"

#pragma region Room Node

UENUM(BlueprintType)
enum class ERoomType : uint8
{
    Base,
    Normal,
    Gimmick,
    MidBoss,
    Boss
};

USTRUCT(BlueprintType)
struct FRoomNode
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RoomID = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERoomType RoomType = ERoomType::Normal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> ConnectedRoomIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int SelectedRoomIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsCleared = false;
};

#pragma endregion

UCLASS()
class UT1_API AUT1_RoomBase : public AActor
{
	GENERATED_BODY()
	
public:
    AUT1_RoomBase();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable)
    virtual void SetupRoom();

    UFUNCTION(BlueprintCallable)
    virtual void ResetRoom();

    UFUNCTION(BlueprintCallable, Category = "Room")
    FVector GetPlayerSpawnLocation() const;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room")
    USceneComponent* PlayerSpawnPoint;
};
