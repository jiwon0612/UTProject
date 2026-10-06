#include "LSW/Widget/UT1_RoomTransitionWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UUT1_RoomTransitionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (WidgetTree)
	{
		UWidget* ExistingRoot = WidgetTree->RootWidget;
		UCanvasPanel* Canvas = Cast<UCanvasPanel>(ExistingRoot);
		if (!Canvas)
		{
			Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TransitionCanvas"));
			WidgetTree->RootWidget = Canvas;

			if (ExistingRoot)
			{
				if (UCanvasPanelSlot* ContentSlot = Canvas->AddChildToCanvas(ExistingRoot))
				{
					ContentSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
					ContentSlot->SetOffsets(FMargin(0.0f));
				}
			}
		}

		BlackOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BlackOverlay"));
		if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(BlackOverlay))
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			CanvasSlot->SetOffsets(FMargin(0.0f));
			CanvasSlot->SetZOrder(10000);
		}
		BlackOverlay->SetBrushColor(FLinearColor::Black);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(0.0f);
}

void UUT1_RoomTransitionWidget::PlayFadeOut()
{
	BeginFade(true);
}

void UUT1_RoomTransitionWidget::PlayFadeIn()
{
	BeginFade(false);
}

void UUT1_RoomTransitionWidget::BeginFade(bool bFadeToBlack)
{
	if (!BlackOverlay)
	{
		UE_LOG(LogTemp, Error, TEXT("[RoomTransition] Black overlay is missing on %s."), *GetName());
		return;
	}

	bFadingOut = bFadeToBlack;
	FadeElapsed = 0.0f;
	FadeStartOpacity = GetRenderOpacity();
	FadeTargetOpacity = bFadeToBlack ? 1.0f : 0.0f;
	bIsFading = true;
	UE_LOG(LogTemp, Log, TEXT("[RoomTransition] %s started on %s."),
		bFadeToBlack ? TEXT("FadeOut") : TEXT("FadeIn"), *GetName());

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeTimerHandle);
		World->GetTimerManager().SetTimer(
			FadeTimerHandle,
			this,
			&UUT1_RoomTransitionWidget::UpdateFade,
			FadeUpdateInterval,
			true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[RoomTransition] No world while starting fade on %s."), *GetName());
	}
}

void UUT1_RoomTransitionWidget::UpdateFade()
{
	if (!bIsFading || !GetWorld())
	{
		return;
	}

	FadeElapsed += FadeUpdateInterval;
	const float Duration = bFadingOut ? FadeOutDuration : FadeInDuration;
	const float Progress = FMath::Clamp(FadeElapsed / Duration, 0.0f, 1.0f);
	const float EasedProgress = FMath::InterpEaseInOut(0.0f, 1.0f, Progress, 2.0f);
	SetRenderOpacity(FMath::Lerp(FadeStartOpacity, FadeTargetOpacity, EasedProgress));

	if (Progress >= 1.0f)
	{
		bIsFading = false;
		GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
		if (bFadingOut)
		{
			OnFadeOutFinished.Broadcast();
		}
		else
		{
			OnFadeInFinished.Broadcast();
		}
	}
}
