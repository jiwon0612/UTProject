#include "LSW/Widget/UT1_PortalWidget.h"

#include "LSW/Rooms/UT1_RoomManager.h"
#include "LSW/Widget/UT1_RoomButton.h"
#include "LSW/Rooms/UT1_RoomBase.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"

void UUT1_PortalWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(true);

    RefreshRoomList();

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateWeakLambda(
                this,
                [this]()
                {
                    if (!IsValid(this))
                        return;

                    SetKeyboardFocus();
                }
            )
        );
    }

}

FReply UUT1_PortalWidget::NativeOnKeyDown(
    const FGeometry& InGeometry,
    const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    if (Key == EKeys::Left)
    {
        MoveSelectionLeft();
        return FReply::Handled();
    }

    if (Key == EKeys::Right)
    {
        MoveSelectionRight();
        return FReply::Handled();
    }

    if (Key == EKeys::SpaceBar)
    {
        MoveSelectedRoom();
        return FReply::Handled();
    }

    if (Key == EKeys::E || Key == EKeys::Escape)
    {
        if (AUT1_RoomManager* RoomManager = GetRoomManager())
        {
            RoomManager->ClosePortalWidget();
        }

        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(
        InGeometry,
        InKeyEvent
    );
}

void UUT1_PortalWidget::MoveSelectionLeft()
{
    AUT1_RoomManager* RoomManager =
        GetRoomManager();

    if (!RoomManager)
        return;

    const TArray<FRoomNode>& RoomNodes =
        RoomManager->GetRoomNodes();

    if (RoomNodes.Num() == 0)
        return;

    const int32 CurrentIndex =
        RoomNodes.IndexOfByPredicate(
            [this](const FRoomNode& Node)
            {
                return Node.RoomID == SelectedRoomID;
            }
        );

    if (CurrentIndex == INDEX_NONE)
        return;

    const int32 NewIndex =
        FMath::Max(0, CurrentIndex - 1);

    SelectedRoomID =
        RoomNodes[NewIndex].RoomID;

    RefreshRoomSelection();
    CenterRoom(SelectedRoomID);
}

void UUT1_PortalWidget::MoveSelectionRight()
{
    AUT1_RoomManager* RoomManager =
        GetRoomManager();

    if (!RoomManager)
        return;

    const TArray<FRoomNode>& RoomNodes =
        RoomManager->GetRoomNodes();

    if (RoomNodes.Num() == 0)
        return;

    const int32 CurrentIndex =
        RoomNodes.IndexOfByPredicate(
            [this](const FRoomNode& Node)
            {
                return Node.RoomID == SelectedRoomID;
            }
        );

    if (CurrentIndex == INDEX_NONE)
        return;

    const int32 NewIndex =
        FMath::Min(
            RoomNodes.Num() - 1,
            CurrentIndex + 1
        );

    SelectedRoomID =
        RoomNodes[NewIndex].RoomID;

    RefreshRoomSelection();
    CenterRoom(SelectedRoomID);
}

void UUT1_PortalWidget::RefreshRoomSelection()
{
    if (!RoomList)
        return;

    const TArray<UWidget*>& Children =
        RoomList->GetAllChildren();

    for (UWidget* Child : Children)
    {
        UUT1_RoomButton* RoomButton =
            Cast<UUT1_RoomButton>(Child);

        if (!RoomButton)
            continue;

        const bool bSelected =
            RoomButton->GetRoomID() == SelectedRoomID;

        RoomButton->SetSelected(bSelected);
    }
}

void UUT1_PortalWidget::MoveSelectedRoom()
{
    AUT1_RoomManager* RoomManager =
        GetRoomManager();

    if (!RoomManager)
        return;

    if (!RoomManager->CanMoveToRoom(SelectedRoomID))
        return;

    RoomManager->MoveToRoom(SelectedRoomID);

    SetVisibility(ESlateVisibility::Collapsed);
}

void UUT1_PortalWidget::RefreshRoomList()
{
    if (!RoomList || !RoomButtonClass)
        return;

    AUT1_RoomManager* RoomManager = GetRoomManager();

    if (!RoomManager)
        return;

    RoomList->ClearChildren();

    const TArray<FRoomNode>& RoomNodes = RoomManager->GetRoomNodes();
    const int32 CurrentRoomID = RoomManager->GetCurrentRoomID();

    SelectedRoomID = CurrentRoomID;

    for (const FRoomNode& RoomNode : RoomNodes)
    {
        UTexture2D* RoomTexture = nullptr;

        switch (RoomNode.RoomType)
        {
        case ERoomType::Base:
            RoomTexture = BaseRoomImage;
            break;

        case ERoomType::Normal:
            RoomTexture = NormalRoomImage;
            break;

        case ERoomType::Reward:
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

        const bool bCanMove = RoomManager->CanMoveToRoom(RoomNode.RoomID);
        const bool bIsCurrent = RoomNode.RoomID == CurrentRoomID;

        UUT1_RoomButton* RoomButton = CreateWidget<UUT1_RoomButton>(
                GetWorld(), RoomButtonClass);

        if (!RoomButton)
            continue;

        RoomButton->SetupRoomButton(RoomNode.RoomID,
            RoomTexture, bCanMove, bIsCurrent);

        RoomButton->OnRoomButtonClicked.AddDynamic(
            this,
            &UUT1_PortalWidget::OnRoomButtonClicked
        );

        UHorizontalBoxSlot* HboxSlot =
            RoomList->AddChildToHorizontalBox(RoomButton);

        if (HboxSlot)
        {
            HboxSlot->SetHorizontalAlignment(HAlign_Center);
            HboxSlot->SetVerticalAlignment(VAlign_Center);

            HboxSlot->SetPadding(
                FMargin(20.0f, 0.0f)
            );
        }
    }

    RefreshRoomSelection();

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateWeakLambda(
                this,
                [this]()
                {
                    if (!IsValid(this) || !RoomScrollBox)
                        return;

                    GetWorld()->GetTimerManager().SetTimerForNextTick(
                        FTimerDelegate::CreateWeakLambda(
                            this,
                            [this]()
                            {
                                if (!IsValid(this))
                                    return;

                                RefreshRoomSelection();
                                CenterRoom(SelectedRoomID);
                            }
                        )
                    );
                }
            )
        );
    }
}

void UUT1_PortalWidget::SetupScrollPadding()
{
    if (!RoomScrollBox ||
        !RoomList)
    {
        return;
    }


    RoomScrollBox->ForceLayoutPrepass();

    RoomList->ForceLayoutPrepass();


    const float ViewWidth =
        RoomScrollBox
        ->GetCachedGeometry()
        .GetLocalSize()
        .X;


    if (ViewWidth <= 0.0f)
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimerForNextTick(
                FTimerDelegate::CreateWeakLambda(
                    this,
                    [this]()
                    {
                        if (!IsValid(this))
                            return;

                        SetupScrollPadding();
                    }
                )
            );
        }

        return;
    }


    const TArray<UWidget*>& Children =
        RoomList->GetAllChildren();


    float ButtonWidth =
        0.0f;


    for (UWidget* Child : Children)
    {
        UUT1_RoomButton* RoomButton =
            Cast<UUT1_RoomButton>(Child);


        if (!RoomButton)
        {
            continue;
        }


        ButtonWidth =
            RoomButton->GetDesiredSize().X;


        break;
    }


    if (ButtonWidth <= 0.0f)
    {
        return;
    }


    constexpr float ButtonPadding =
        10.0f;


    const float SpacerWidth =
        FMath::Max(
            0.0f,
            (ViewWidth -
                (ButtonWidth + ButtonPadding))
            * 0.5f
        );

    RoomList->ForceLayoutPrepass();


    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateWeakLambda(
                this,
                [this]()
                {
                    if (!IsValid(this))
                        return;

                    CenterCurrentRoom();
                }
            )
        );
    }
}

void UUT1_PortalWidget::CenterCurrentRoom()
{
    CenterRoom(
        SelectedRoomID
    );
}

void UUT1_PortalWidget::CenterRoom(int32 RoomID)
{
    if (!RoomScrollBox || !RoomList)
        return;

    const TArray<UWidget*>& Children =
        RoomList->GetAllChildren();

    UWidget* TargetWidget = nullptr;

    for (UWidget* Child : Children)
    {
        UUT1_RoomButton* RoomButton =
            Cast<UUT1_RoomButton>(Child);

        if (!RoomButton)
            continue;

        if (RoomButton->GetRoomID() == RoomID)
        {
            TargetWidget = RoomButton;
            break;
        }
    }

    if (!TargetWidget)
        return;

    RoomScrollBox->ScrollWidgetIntoView(
        TargetWidget,
        true,
        EDescendantScrollDestination::Center
    );
}
bool UUT1_PortalWidget::CanMoveToRoom(int32 RoomID) const
{
    AUT1_RoomManager* RoomManager = GetRoomManager();

    if (!RoomManager)
    {
        return false;
    }

    const FRoomNode* CurrentRoom =
        RoomManager->GetCurrentRoomNode();

    if (!CurrentRoom)
    {
        return false;
    }

    const FRoomNode* TargetRoom = RoomManager->FindRoomNode(
            RoomID);

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

    if (TargetRoom->RoomType ==
        ERoomType::Base)
    {
        return TargetRoom->bIsCleared;
    }

    if (TargetRoom->RoomType ==
        ERoomType::MidBoss)
    {
        return TargetRoom->bIsCleared;
    }


    return false;
}

void UUT1_PortalWidget::OnRoomButtonClicked(int32 RoomID)
{
    SelectedRoomID = RoomID;

    RefreshRoomSelection();
    CenterRoom(SelectedRoomID);
}

AUT1_RoomManager* UUT1_PortalWidget::GetRoomManager() const
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
