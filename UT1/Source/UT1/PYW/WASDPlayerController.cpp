#include "PYW/WASDPlayerController.h"

#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

AWASDPlayerController::AWASDPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableTouchEvents = false;
}

void AWASDPlayerController::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Display, TEXT("PYW_INPUT WASD controller active; click movement disabled"));
}

void AWASDPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	StopMovement();
}

void AWASDPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || IsPaused() || !IsLocalController())
	{
		return;
	}

	const float ForwardInput = (IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f)
		- (IsInputKeyDown(EKeys::S) ? 1.0f : 0.0f);
	const float RightInput = (IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f)
		- (IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f);
	FVector MoveDirection(ForwardInput, RightInput, 0.0f);
	if (!MoveDirection.IsNearlyZero())
	{
		ControlledPawn->AddMovementInput(MoveDirection.GetSafeNormal(), 1.0f);
	}
}
