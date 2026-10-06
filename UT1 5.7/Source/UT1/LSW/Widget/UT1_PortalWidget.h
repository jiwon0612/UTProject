#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UT1_PortalWidget.generated.h"

class AUT1_RoomManager;
class UCanvasPanel;
class UTextBlock;
class UButton;
class UUT1_RoomButton;

UCLASS()
class UT1_API UUT1_PortalWidget : public UUserWidget
{
    GENERATED_BODY()

protected:

    virtual void NativeConstruct() override;

public:

    UPROPERTY(meta = (BindWidget))
    UButton* NextRoomButton;

    UPROPERTY(meta = (BindWidget))
    UButton* BaseButton;


    UFUNCTION()
    void OnNextRoomClicked();

    UFUNCTION()
    void OnBaseClicked();


private:

    AUT1_RoomManager* GetRoomManager() const;
};