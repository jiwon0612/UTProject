#include "LSW/Widget/UT1_RoomButton.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"


void UUT1_RoomButton::NativeConstruct()
{
    Super::NativeConstruct();

    if (RoomButton)
    {
        RoomButton->OnClicked.AddDynamic(
            this,
            &UUT1_RoomButton::OnClicked
        );
    }
}


void UUT1_RoomButton::SetupRoomButton(int32 InRoomID, const FString& InRoomName,
    bool bCanMove, bool bIsCurrent)
{
    RoomID = InRoomID;

    if (RoomText)
    {
        FString DisplayText;

        DisplayText =
            FString::Printf(
                TEXT("%d : %s"),
                RoomID,
                *InRoomName
            );

        RoomText->SetText(
            FText::FromString(DisplayText)
        );
    }

    if (RoomButton)
    {
        RoomButton->SetIsEnabled(
            bCanMove
        );
    }
}


void UUT1_RoomButton::OnClicked()
{
    OnRoomButtonClicked.Broadcast(
        RoomID
    );
}