// Copyright Epic Games, Inc. All Rights Reserved.

#include "BuffMessageUI.h"
#include "Blueprint/WidgetTree.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"

void UBuffMessageUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	if (!Canvas)
	{
		return;
	}
	WidgetTree->RootWidget = Canvas;

	// Background border for visibility
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	if (BackgroundBorder)
	{
		BackgroundBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
		BackgroundBorder->SetVisibility(ESlateVisibility::Collapsed);

		UCanvasPanelSlot* BorderSlot = Canvas->AddChildToCanvas(BackgroundBorder);
		if (BorderSlot)
		{
			BorderSlot->SetAnchors(FAnchors(0.1f, 0.35f, 0.9f, 0.55f));
			BorderSlot->SetOffsets(FMargin(0.0f));
			BorderSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		}
	}

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageText"));
	if (MessageText)
	{
		MessageText->SetJustification(ETextJustify::Center);
		MessageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));

		FSlateFontInfo FontInfo;
		FontInfo.Size = 48;
		MessageText->SetFont(FontInfo);

		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(MessageText);
		if (CanvasSlot)
		{
			CanvasSlot->SetAnchors(FAnchors(0.1f, 0.35f, 0.9f, 0.55f));
			CanvasSlot->SetOffsets(FMargin(20.0f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		}
	}

	SetVisibility(ESlateVisibility::Hidden);
}

void UBuffMessageUI::Show(const FString& Message, FColor Color)
{
	if (MessageText)
	{
		MessageText->SetText(FText::FromString(Message));
		MessageText->SetColorAndOpacity(FSlateColor(Color));
	}
	if (BackgroundBorder)
	{
		BackgroundBorder->SetVisibility(ESlateVisibility::Visible);
	}
	SetVisibility(ESlateVisibility::Visible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
		World->GetTimerManager().SetTimer(HideTimer, this, &UBuffMessageUI::Hide, 2.0f, false);
	}
}

void UBuffMessageUI::ShowPersistent(const FString& Message, FColor Color)
{
	if (MessageText)
	{
		MessageText->SetText(FText::FromString(Message));
		MessageText->SetColorAndOpacity(FSlateColor(Color));
	}
	if (BackgroundBorder)
	{
		BackgroundBorder->SetVisibility(ESlateVisibility::Visible);
	}
	SetVisibility(ESlateVisibility::Visible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
	}
}

void UBuffMessageUI::Hide()
{
	if (BackgroundBorder)
	{
		BackgroundBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetVisibility(ESlateVisibility::Hidden);
}
