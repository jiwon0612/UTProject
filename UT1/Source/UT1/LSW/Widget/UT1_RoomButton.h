#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1_RoomButton.generated.h"

class UButton;
class UImage;
class USizeBox;
class UOverlay;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnRoomButtonClicked,
    int32,
    RoomID
);

UCLASS()
class UT1_API UUT1_RoomButton : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    UButton* RoomButton;

    UPROPERTY(meta = (BindWidget))
    UImage* RoomImage;

    UPROPERTY(meta = (BindWidget))
    USizeBox* RoomSizeBox;

public:
    UFUNCTION(BlueprintCallable)
    void SetupRoomButton(
        int32 InRoomID,
        UTexture2D* InRoomTexture,
        bool bInCanMove,
        bool bInIsCurrent
    );

    UFUNCTION(BlueprintCallable)
    void SetSelected(bool bSelected);

    UFUNCTION(BlueprintCallable)
    void UpdateRoomColor();

    UFUNCTION(BlueprintPure)
    int32 GetRoomID() const
    {
        return RoomID;
    }

    UFUNCTION(BlueprintPure)
    bool IsSelected() const
    {
        return bIsSelected;
    }

    UPROPERTY(BlueprintAssignable)
    FOnRoomButtonClicked OnRoomButtonClicked;

    UPROPERTY(meta = (BindWidget))
    UImage* DisabledOverlay;

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleRoomButtonClicked();

private:
    UPROPERTY()
    int32 RoomID = INDEX_NONE;

    UPROPERTY()
    bool bCanMove = false;

    UPROPERTY()
    bool bIsCurrent = false;

    UPROPERTY()
    bool bIsSelected = false;
};