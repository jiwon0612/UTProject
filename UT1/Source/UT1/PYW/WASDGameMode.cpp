#include "PYW/WASDGameMode.h"

#include "PYW/WASDPlayerController.h"
#include "UObject/ConstructorHelpers.h"

AWASDGameMode::AWASDGameMode()
{
	PlayerControllerClass = AWASDPlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClass(
		TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (PlayerPawnClass.Succeeded())
	{
		DefaultPawnClass = PlayerPawnClass.Class;
	}
}
