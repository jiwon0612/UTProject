// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UT1_Portal.generated.h"

class UBoxComponent;

UCLASS()
class UT1_API AUT1_Portal : public AActor
{
	GENERATED_BODY()
	
public:

    AUT1_Portal();

protected:

    UPROPERTY(VisibleAnywhere)
    UBoxComponent* Collision;

public:

    UPROPERTY(BlueprintReadWrite)
    int32 TargetRoomID;

    UFUNCTION()
    void OnPortalOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
