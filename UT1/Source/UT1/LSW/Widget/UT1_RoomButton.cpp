#include "LSW/Widget/UT1_RoomButton.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"

void UUT1_RoomButton::NativeConstruct()
{
    Super::NativeConstruct();

    if (RoomButton)
    {
        RoomButton->OnClicked.AddDynamic(
            this,
            &UUT1_RoomButton::HandleRoomButtonClicked
        );
    }

    SetSelected(false);
}

void UUT1_RoomButton::SetupRoomButton(
    int32 InRoomID,
    UTexture2D* InRoomTexture,
    bool bInCanMove,
    bool bInIsCurrent
)
{
    RoomID = InRoomID;
    bCanMove = bInCanMove;
    bIsCurrent = bInIsCurrent;

    if (RoomImage)
    {
        RoomImage->SetBrushFromTexture(InRoomTexture);
    }

    UpdateRoomColor();
    SetSelected(false);
}

void UUT1_RoomButton::SetSelected(bool bSelected)
{
    bIsSelected = bSelected;

    if (!RoomSizeBox)
        return;

    RoomSizeBox->SetWidthOverride(180.0f);
    RoomSizeBox->SetHeightOverride(180.0f);

    FWidgetTransform Transform;

    Transform.Scale = bIsSelected
        ? FVector2D(1.5f, 1.5f)
        : FVector2D(1.0f, 1.0f);

    SetRenderTransform(Transform);
}

void UUT1_RoomButton::UpdateRoomColor()
{
    if (!RoomImage)
        return;

    if (bIsCurrent)
    {
        // 현재 방
        RoomImage->SetColorAndOpacity(
            FLinearColor(1.0f, 0.8f, 0.2f, 1.0f)
        );
    }
    else if (!bCanMove)
    {
        // 이동 불가능
        RoomImage->SetColorAndOpacity(
            FLinearColor(0.35f, 0.35f, 0.35f, 1.0f)
        );
    }
    else
    {
        // 이동 가능
        RoomImage->SetColorAndOpacity(
            FLinearColor::White
        );
    }
}

void UUT1_RoomButton::HandleRoomButtonClicked()
{
    if (RoomID == INDEX_NONE)
        return;

    if (!bCanMove && !bIsCurrent)
        return;

    OnRoomButtonClicked.Broadcast(RoomID);
}