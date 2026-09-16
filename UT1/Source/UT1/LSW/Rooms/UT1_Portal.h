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

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 TargetRoomID = INDEX_NONE;

    bool IsPlayerInRange() const
    {
        UWorld* World = GetWorld();

        if (!World)
        {
            return false;
        }

        APlayerController* PC =
            World->GetFirstPlayerController();

        if (!PC)
        {
            return false;
        }

        APawn* PlayerPawn =
            PC->GetPawn();

        if (!PlayerPawn)
        {
            return false;
        }

        const float Distance =
            FVector::Dist(
                GetActorLocation(),
                PlayerPawn->GetActorLocation()
            );

        return Distance <= 200.f;
    }
};