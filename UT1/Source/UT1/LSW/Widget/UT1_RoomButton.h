#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1_RoomButton.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnRoomButtonClicked,
    int32,
    RoomID
);

UCLASS()
class UT1_API UUT1_RoomButton : public UUserWidget
{
    GENERATED_BODY()

protected:

    virtual void NativeConstruct() override;

public:

    UPROPERTY(meta = (BindWidget))
    UButton* RoomButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* RoomText;


    UPROPERTY(BlueprintAssignable)
    FOnRoomButtonClicked OnRoomButtonClicked;


    void SetupRoomButton(
        int32 InRoomID,
        const FString& InRoomName,
        bool bCanMove,
        bool bIsCurrent
    );

public:
    int32 GetRoomID() const
    {
        return RoomID;
    }

private:

    UFUNCTION()
    void OnClicked();


private:

    int32 RoomID = INDEX_NONE;
};