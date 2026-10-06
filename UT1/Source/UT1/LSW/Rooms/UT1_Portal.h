#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CJW/Interaction/UT1Interactable.h"
#include "UT1_Portal.generated.h"

class AUT1Player;
class USphereComponent;

UCLASS()
class UT1_API AUT1_Portal : public AActor, public IUT1Interactable
{
    GENERATED_BODY()

public:

    AUT1_Portal();

    virtual void Interact_Implementation(AUT1Player* Interactor) override;

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 TargetRoomID = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
    TObjectPtr<USphereComponent> InteractRange;

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
