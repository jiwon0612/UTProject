#include "LSW/Widget/UT1_RoomButton.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"


void UUT1_RoomButton::NativeConstruct()
{
    Super::NativeConstruct();

    if (RoomButton)
    {
        RoomButton->OnClicked.Clear();

        RoomButton->OnClicked.AddDynamic(
            this,
            &UUT1_RoomButton::OnClicked
        );
    }

    UpdateButtonState();
}


// =============================================================
// Initialize
// =============================================================

void UUT1_RoomButton::InitializeRoom(
    int32 InRoomID,
    bool bInCanMove,
    bool bInIsCurrent
)
{
    RoomID = InRoomID;

    bCanMove = bInCanMove;

    bIsCurrent = bInIsCurrent;

    if (RoomText)
    {
        if (bIsCurrent)
        {
            RoomText->SetText(
                FText::FromString(
                    FString::Printf(
                        TEXT("현재\nRoom %d"),
                        RoomID
                    )
                )
            );
        }
        else if (bCanMove)
        {
            RoomText->SetText(
                FText::FromString(
                    FString::Printf(
                        TEXT("이동\nRoom %d"),
                        RoomID
                    )
                )
            );
        }
        else
        {
            RoomText->SetText(
                FText::FromString(
                    FString::Printf(
                        TEXT("Room %d"),
                        RoomID
                    )
                )
            );
        }
    }

    UpdateButtonState();
}


// =============================================================
// Button State
// =============================================================

void UUT1_RoomButton::UpdateButtonState()
{
    if (!RoomButton)
    {
        return;
    }

    /*
     * 현재 방
     * → 클릭 불가
     *
     * 연결된 방
     * → 클릭 가능
     *
     * 연결되지 않은 방
     * → 클릭 불가
     */

    if (bIsCurrent)
    {
        RoomButton->SetIsEnabled(false);
    }
    else
    {
        RoomButton->SetIsEnabled(bCanMove);
    }
}


// =============================================================
// Click
// =============================================================

void UUT1_RoomButton::OnClicked()
{
    if (!bCanMove)
    {
        return;
    }

    if (bIsCurrent)
    {
        return;
    }

    if (RoomID == INDEX_NONE)
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[RoomButton] Clicked Room %d"
        ),
        RoomID
    );

    OnRoomButtonClicked.Broadcast(
        RoomID
    );
}