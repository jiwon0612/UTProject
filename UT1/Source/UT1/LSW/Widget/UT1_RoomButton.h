#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1_RoomButton.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FRoomButtonClicked,
    int32,
    RoomID
);

UCLASS()
class UT1_API UUT1_RoomButton : public UUserWidget
{
    GENERATED_BODY()

public:

    virtual void NativeConstruct() override;

    // =========================================================
    // 초기화
    // =========================================================

    void InitializeRoom(
        int32 InRoomID,
        bool bInCanMove,
        bool bInIsCurrent
    );

public:

    UPROPERTY(BlueprintAssignable)
    FRoomButtonClicked OnRoomButtonClicked;

protected:

    // =========================================================
    // WBP_RoomButton
    // =========================================================

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RoomButton;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> RoomText;


private:

    UFUNCTION()
    void OnClicked();


    void UpdateButtonState();


private:

    int32 RoomID = INDEX_NONE;

    bool bCanMove = false;

    bool bIsCurrent = false;
};