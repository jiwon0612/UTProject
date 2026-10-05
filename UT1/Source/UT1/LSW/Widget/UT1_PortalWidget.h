#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1_PortalWidget.generated.h"

class AUT1_RoomManager;
class UHorizontalBox;
class UScrollBox;
class USpacer;
class UUT1_RoomButton;

UCLASS()
class UT1_API UUT1_PortalWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:
    UPROPERTY(meta = (BindWidget))
    UScrollBox* RoomScrollBox;

    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* RoomList;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
    TSubclassOf<UUT1_RoomButton> RoomButtonClass;

    UFUNCTION(BlueprintCallable)
    void RefreshRoomList();

private:
    UFUNCTION()
    void OnRoomButtonClicked(int32 RoomID);

    AUT1_RoomManager* GetRoomManager() const;

    FString GetRoomTypeName(int32 RoomID) const;

    bool CanMoveToRoom(int32 RoomID) const;

    // 좌우 빈 공간 설정
    void SetupScrollPadding();

    // 현재 방을 가운데로 이동
    void CenterCurrentRoom();

private:
    UPROPERTY()
    USpacer* LeftSpacer = nullptr;

    UPROPERTY()
    USpacer* RightSpacer = nullptr;
};