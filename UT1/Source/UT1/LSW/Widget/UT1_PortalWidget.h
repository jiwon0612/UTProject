#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1_PortalWidget.generated.h"

class AUT1_RoomManager;
class UHorizontalBox;
class UScrollBox;
class USpacer;
class UUT1_RoomButton;
class UTexture2D;

UCLASS()
class UT1_API UUT1_PortalWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry,
        const FKeyEvent& InKeyEvent) override;

    void MoveSelectionRight();
    void MoveSelectionLeft();

public:
    UFUNCTION(BlueprintCallable)
    void RefreshRoomList();

public:
    UPROPERTY(meta = (BindWidget))
    UScrollBox* RoomScrollBox;

    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* RoomList;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
    TSubclassOf<UUT1_RoomButton> RoomButtonClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room Image")
    UTexture2D* BaseRoomImage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room Image")
    UTexture2D* NormalRoomImage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room Image")
    UTexture2D* RewardRoomImage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room Image")
    UTexture2D* MidBossRoomImage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room Image")
    UTexture2D* BossRoomImage;

private:
    UFUNCTION()
    void OnRoomButtonClicked(int32 RoomID);

    AUT1_RoomManager* GetRoomManager() const;

    bool CanMoveToRoom(int32 RoomID) const;

    void CenterRoom(int32 RoomID);

    UWidget* FindRoomButton(int32 RoomID) const;

private:
    int32 SelectedRoomID = INDEX_NONE;
};