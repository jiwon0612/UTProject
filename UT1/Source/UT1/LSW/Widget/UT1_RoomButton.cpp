#include "LSW/Widget/UT1_RoomButton.h"

#include "Components/Button.h"
#include "Components/Image.h"

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

void UUT1_RoomButton::SetupRoomButton(int32 InRoomID, UTexture2D* InRoomTexture,
    bool bCanMove, bool bIsCurrent
)
{
    RoomID = InRoomID;

    if (RoomImage && InRoomTexture)
    {
        RoomImage->SetBrushFromTexture(InRoomTexture);
    }

    if (RoomButton)
    {
        RoomButton->SetIsEnabled(bCanMove);

        if (bIsCurrent)
        {
            RoomImage->SetColorAndOpacity(
                FLinearColor::White
            );
        }
        else if (bCanMove)
        {
            RoomImage->SetColorAndOpacity(
                FLinearColor(0.8f, 0.8f, 0.8f, 1.0f)
            );
        }
        else
        {
            RoomImage->SetColorAndOpacity(
                FLinearColor(0.4f, 0.4f, 0.4f, 1.0f)
            );
        }
    }
}

void UUT1_RoomButton::OnClicked()
{
    OnRoomButtonClicked.Broadcast(RoomID);
}