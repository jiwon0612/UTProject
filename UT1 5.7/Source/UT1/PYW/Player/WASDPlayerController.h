#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WASDPlayerController.generated.h"

/** Keyboard-only movement controller for the PYW top-down test level. */
UCLASS()
class UT1_API AWASDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWASDPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void PlayerTick(float DeltaTime) override;
};
