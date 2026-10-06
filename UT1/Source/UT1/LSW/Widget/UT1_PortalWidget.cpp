#include "LSW/Widget/UT1_PortalWidget.h"

#include "LSW/Rooms/UT1_RoomManager.h"
#include "LSW/Rooms/UT1_RoomBase.h"
#include "LSW/Widget/UT1_RoomButton.h"

#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"

#include "Kismet/GameplayStatics.h"

void UUT1_PortalWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UUT1_PortalWidget::RefreshRoomList()
{
    AUT1_RoomManager* RoomManager = GetRoomManager();

    if (!RoomManager)
    {
        return;
    }

    if (!RoomList)
    {
        return;
    }

    if (!RoomScrollBox)
    {
        return;
    }

    if (!RoomButtonClass)
    {
        return;
    }

    RoomList->ClearChildren();

    LeftSpacer = nullptr;
    RightSpacer = nullptr;

    const TArray<FRoomNode>& RoomNodes = RoomManager->GetRoomNodes();
    const int32 CurrentRoomID = RoomManager->GetCurrentRoomID();

    LeftSpacer = NewObject<USpacer>(this);

    if (LeftSpacer)
    {
        UHorizontalBoxSlot* SpacerSlot = RoomList->AddChildToHorizontalBox(
                LeftSpacer);

        if (SpacerSlot)
        {
            SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    for (const FRoomNode& RoomNode : RoomNodes)
    {
        if (RoomNode.RoomID == INDEX_NONE)
        {
            continue;
        }

        UUT1_RoomButton* RoomButton = CreateWidget<UUT1_RoomButton>(
                GetWorld(), RoomButtonClass);

        if (!RoomButton)
        {
            continue;
        }

        const bool bIsCurrent = RoomNode.RoomID == CurrentRoomID;
        const bool bCanMove = CanMoveToRoom(RoomNode.RoomID);
        UTexture2D* RoomTexture = nullptr;

        switch (RoomNode.RoomType)
        {
        case ERoomType::Base:
            RoomTexture = BaseRoomImage;
            break;

        case ERoomType::Normal:
            RoomTexture = NormalRoomImage;
            break;

        case ERoomType::Gimmick:
            RoomTexture = RewardRoomImage;
            break;

        case ERoomType::MidBoss:
            RoomTexture = MidBossRoomImage;
            break;

        case ERoomType::Boss:
            RoomTexture = BossRoomImage;
            break;

        default:
            break;
        }

        RoomButton->SetupRoomButton(
            RoomNode.RoomID,
            RoomTexture,
            bCanMove,
            bIsCurrent
        );

        RoomButton->OnRoomButtonClicked.AddDynamic(this, &UUT1_PortalWidget::OnRoomButtonClicked);

        UHorizontalBoxSlot* HorizontalSlot =
            RoomList->AddChildToHorizontalBox(RoomButton);

        if (HorizontalSlot)
        {
            HorizontalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            HorizontalSlot->SetHorizontalAlignment(HAlign_Left);
            HorizontalSlot->SetVerticalAlignment(VAlign_Center);
            HorizontalSlot->SetPadding(FMargin(5.0f));
        }
    }

    RightSpacer = NewObject<USpacer>(this);

    if (RightSpacer)
    {
        UHorizontalBoxSlot* SpacerSlot = RoomList->AddChildToHorizontalBox(RightSpacer);

        if (SpacerSlot)
        {
            SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        this, &UUT1_PortalWidget::SetupScrollPadding);
}


void UUT1_PortalWidget::SetupScrollPadding()
{
    if (!RoomScrollBox || !RoomList ||
        !LeftSpacer || !RightSpacer)
    {
        return;
    }

    RoomScrollBox->ForceLayoutPrepass();
    RoomList->ForceLayoutPrepass();

    const float ViewWidth = RoomScrollBox
        ->GetCachedGeometry().GetLocalSize().X;

    if (ViewWidth <= 0.0f)
    {

        GetWorld()->GetTimerManager().SetTimerForNextTick(this,
            &UUT1_PortalWidget::SetupScrollPadding);
        return;
    }

    const TArray<UWidget*>& Children = RoomList->GetAllChildren();

    float ButtonWidth = 0.0f;

    for (UWidget* Child : Children)
    {
        UUT1_RoomButton* RoomButton = Cast<UUT1_RoomButton>(Child);

        if (!RoomButton)
        {
            continue;
        }

        ButtonWidth = RoomButton->GetDesiredSize().X;
        break;
    }

    if (ButtonWidth <= 0.0f)
    {
        return;
    }

    constexpr float ButtonPadding = 10.0f;
    const float SpacerWidth = FMath::Max(0.0f,
            (ViewWidth - (ButtonWidth + ButtonPadding)) * 0.5f);

    LeftSpacer->SetSize(FVector2D(
        SpacerWidth, 1.0f));

    RightSpacer->SetSize(FVector2D(
            SpacerWidth, 1.0f));

    RoomList->ForceLayoutPrepass();

    GetWorld()->GetTimerManager().SetTimerForNextTick(this,
        &UUT1_PortalWidget::CenterCurrentRoom);
}

void UUT1_PortalWidget::CenterCurrentRoom()
{
    if (!RoomScrollBox || !RoomList)
    {
        return;
    }

    AUT1_RoomManager* RoomManager = GetRoomManager();

    if (!RoomManager)
    {
        return;
    }

    const int32 CurrentRoomID = RoomManager->GetCurrentRoomID();


    const TArray<UWidget*>& Children =
        RoomList->GetAllChildren();

    UUT1_RoomButton* CurrentButton = nullptr;

    for (UWidget* Child : Children)
    {
        UUT1_RoomButton* RoomButton = Cast<UUT1_RoomButton>(Child);

        if (!RoomButton)
        {
            continue;
        }

        if (RoomButton->GetRoomID() == CurrentRoomID)
        {
            CurrentButton = RoomButton;
            break;
        }
    }

    if (!CurrentButton)
    {
        return;
    }

    RoomScrollBox->ScrollWidgetIntoView(CurrentButton,
        false, EDescendantScrollDestination::Center);
}

bool UUT1_PortalWidget::CanMoveToRoom(int32 RoomID) const
{
    AUT1_RoomManager* RoomManager = GetRoomManager();

    if (!RoomManager)
    {
        return false;
    }

    const FRoomNode* CurrentRoom = RoomManager->GetCurrentRoomNode();

    if (!CurrentRoom)
    {
        return false;
    }

    const FRoomNode* TargetRoom = RoomManager->FindRoomNode(RoomID);

    if (!TargetRoom)
    {
        return false;
    }

    if (RoomID == CurrentRoom->RoomID)
    {
        return false;
    }

    if (RoomID == CurrentRoom->RoomID + 1)
    {
        return true;
    }

    if (TargetRoom->RoomType == ERoomType::Base)
    {
        return TargetRoom->bIsCleared;
    }

    if (TargetRoom->RoomType == ERoomType::MidBoss)
    {
        return TargetRoom->bIsCleared;
    }

    return false;
}


FString UUT1_PortalWidget::GetRoomTypeName(int32 RoomID) const
{
    AUT1_RoomManager* RoomManager = GetRoomManager();

    if (!RoomManager)
    {
        return TEXT("Unknown");
    }

    const FRoomNode* RoomNode =
        RoomManager->FindRoomNode(RoomID);

    if (!RoomNode)
    {
        return TEXT("Unknown");
    }

    switch (RoomNode->RoomType)
    {
        case ERoomType::Base:
            return TEXT("Base");

        case ERoomType::Normal:
            return TEXT("Normal");

        case ERoomType::Gimmick:
            return TEXT("Reward");

        case ERoomType::MidBoss:
            return TEXT("Mid Boss");

        case ERoomType::Boss:
            return TEXT("Boss");

        default:
            return TEXT("Unknown");
    }
}

void UUT1_PortalWidget::OnRoomButtonClicked(int32 RoomID)
{
    AUT1_RoomManager* RoomManager = GetRoomManager();

    if (!RoomManager)
    {
        return;
    }

    if (!RoomManager->CanMoveToRoom(RoomID))
    {
        return;
    }

    RoomManager->MoveToRoom(RoomID);

    SetVisibility(ESlateVisibility::Collapsed);
}


AUT1_RoomManager* UUT1_PortalWidget::GetRoomManager() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    return Cast<AUT1_RoomManager>(UGameplayStatics::GetActorOfClass(
            GetWorld(), AUT1_RoomManager::StaticClass()));
}