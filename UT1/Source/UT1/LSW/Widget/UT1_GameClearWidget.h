#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1_GameClearWidget.generated.h"

/** Final boss victory overlay. A Blueprint subclass can replace the native fallback layout. */
UCLASS()
class UT1_API UUT1_GameClearWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Show();

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildDefaultLayout();
};
