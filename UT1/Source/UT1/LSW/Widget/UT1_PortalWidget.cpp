#include "LSW/Widget/UT1_PortalWidget.h"

#include "LSW/Rooms/UT1_RoomManager.h"
#include "LSW/Rooms/UT1_RoomBase.h"
#include "LSW/Widget/UT1_RoomButton.h"

#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"

#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"

void UUT1_PortalWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(true);

    RefreshRoomList();

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                SetKeyboardFocus();
            })
    );
}

FReply UUT1_PortalWidget::NativeOnKeyDown(const FGeometry& InGeometry,
    const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    if (Key == EKeys::Right)

    {
        MoveSelectionRight();
        return FReply::Handled();
    }

    if (Key == EKeys::Left)
    {
        MoveSelectionLeft();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(
        InGeometry,
        InKeyEvent
    );
}

void UUT1_PortalWidget::MoveSelectionRight()
{
    AUT1_RoomManager* RoomManager =
        GetRoomManager();

    if (!RoomManager)
        return;

    const int32 NextRoomID =
        SelectedRoomID + 1;

    const FRoomNode* NextRoom =
        RoomManager->FindRoomNode(NextRoomID);

    if (!NextRoom)
        return;

    SelectedRoomID = NextRoomID;

    CenterRoom(SelectedRoomID);
}

void UUT1_PortalWidget::MoveSelectionLeft()
{
    AUT1_RoomManager* RoomManager =
        GetRoomManager();

    if (!RoomManager)
        return;

    const int32 PreviousRoomID =
        SelectedRoomID - 1;

    const FRoomNode* PreviousRoom =
        RoomManager->FindRoomNode(PreviousRoomID);

    if (!PreviousRoom)
        return;

    SelectedRoomID = PreviousRoomID;

    CenterRoom(SelectedRoomID);
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

    const TArray<FRoomNode>& RoomNodes = RoomManager->GetRoomNodes();
    const int32 CurrentRoomID = RoomManager->GetCurrentRoomID();
    SelectedRoomID = CurrentRoomID;

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

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                CenterRoom(SelectedRoomID);
            })
    );
}

void UUT1_PortalWidget::CenterRoom(int32 RoomID)
{
    if (!RoomScrollBox || !RoomList)
        return;

    UWidget* TargetWidget = FindRoomButton(RoomID);

    if (!TargetWidget)
        return;

    RoomScrollBox->ForceLayoutPrepass();
    RoomList->ForceLayoutPrepass();

    const float ViewWidth = RoomScrollBox->GetCachedGeometry().GetLocalSize().X;
    const float ContentWidth = RoomList->GetCachedGeometry().GetLocalSize().X;
    const float TargetWidth = TargetWidget->GetCachedGeometry().GetLocalSize().X;

    if (ViewWidth <= 0.0f || ContentWidth <= 0.0f ||
        TargetWidth <= 0.0f)
    {
        return;
    }

    const FVector2D ScrollAbsolute = RoomScrollBox->GetCachedGeometry()
        .LocalToAbsolute(FVector2D::ZeroVector);
    const FVector2D TargetAbsolute = TargetWidget->GetCachedGeometry()
        .LocalToAbsolute(FVector2D::ZeroVector);
    const float TargetX = TargetAbsolute.X - ScrollAbsolute.X;
    const float TargetCenter = TargetX + TargetWidth * 0.5f;
    const float CurrentScrollOffset = RoomScrollBox->GetScrollOffset();
    const float TargetScrollOffset = CurrentScrollOffset
        + TargetCenter - ViewWidth * 0.5f;
    const float MaxScrollOffset = FMath::Max(0.0f,
            ContentWidth - ViewWidth);
    const float FinalOffset = FMath::Clamp(TargetScrollOffset,
            0.0f, MaxScrollOffset);

    RoomScrollBox->SetScrollOffset(FinalOffset);
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

UWidget* UUT1_PortalWidget::FindRoomButton(int32 RoomID) const
{
    if (!RoomList)
        return nullptr;

    const TArray<UWidget*>& Children =
        RoomList->GetAllChildren();

    for (UWidget* Child : Children)
    {
        UUT1_RoomButton* RoomButton =
            Cast<UUT1_RoomButton>(Child);

        if (!RoomButton)
            continue;

        if (RoomButton->GetRoomID() == RoomID)
        {
            return RoomButton;
        }
    }

    return nullptr;
}