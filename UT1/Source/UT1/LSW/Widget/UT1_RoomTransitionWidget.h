#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1_RoomTransitionWidget.generated.h"

class UBorder;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUT1RoomTransitionFinished);

/** Full-screen black overlay that reports when each fade has finished. */
UCLASS()
class UT1_API UUT1_RoomTransitionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Room Transition")
	void PlayFadeOut();

	UFUNCTION(BlueprintCallable, Category = "Room Transition")
	void PlayFadeIn();

	UPROPERTY(BlueprintAssignable, Category = "Room Transition")
	FUT1RoomTransitionFinished OnFadeOutFinished;

	UPROPERTY(BlueprintAssignable, Category = "Room Transition")
	FUT1RoomTransitionFinished OnFadeInFinished;

protected:
	virtual void NativeConstruct() override;

private:
	void BeginFade(bool bFadeToBlack);
	void UpdateFade();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BlackOverlay;

	UPROPERTY(EditDefaultsOnly, Category = "Room Transition", meta = (ClampMin = "0.01"))
	float FadeOutDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Room Transition", meta = (ClampMin = "0.01"))
	float FadeInDuration = 0.35f;

	float FadeElapsed = 0.0f;
	float FadeStartOpacity = 0.0f;
	float FadeTargetOpacity = 0.0f;
	bool bIsFading = false;
	bool bFadingOut = false;
	FTimerHandle FadeTimerHandle;
	static constexpr float FadeUpdateInterval = 1.0f / 60.0f;
};
