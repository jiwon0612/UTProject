#include "LSW/Widget/UT1_PortalWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "LSW/Rooms/UT1_RoomManager.h"


void UUT1_PortalWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (NextRoomButton)
    {
        NextRoomButton->OnClicked.AddDynamic(
            this,
            &UUT1_PortalWidget::OnNextRoomClicked
        );
    }

    if (BaseButton)
    {
        BaseButton->OnClicked.AddDynamic(
            this,
            &UUT1_PortalWidget::OnBaseClicked
        );
    }
}

void UUT1_PortalWidget::OnNextRoomClicked()
{
    AUT1_RoomManager* RoomManager =
        GetRoomManager();

    if (!RoomManager)
    {
        return;
    }

    RoomManager->MoveToNextRoom();

    SetVisibility(ESlateVisibility::Collapsed);
}

void UUT1_PortalWidget::OnBaseClicked()
{
    AUT1_RoomManager* RoomManager =
        GetRoomManager();

    if (!RoomManager)
    {
        return;
    }

    RoomManager->MoveToBaseRoom();

    SetVisibility(ESlateVisibility::Collapsed);
}

AUT1_RoomManager*
UUT1_PortalWidget::GetRoomManager() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    return Cast<AUT1_RoomManager>(
        UGameplayStatics::GetActorOfClass(
            GetWorld(),
            AUT1_RoomManager::StaticClass()
        )
    );
}