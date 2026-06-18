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

	// Auto-find the first TextBlock from user's blueprint (any name works)
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (!MessageText)
			{
				MessageText = Cast<UTextBlock>(Widget);
			}
		});
	}

	SetVisibility(ESlateVisibility::Hidden);
}

void UBuffMessageUI::Show(const FString& Message, FColor Color)
{
	if (MessageText)
	{
		// Append new message on top, keep previous ones below
		const FText Existing = MessageText->GetText();
		const FText NewLine = FText::FromString(Message);
		if (Existing.IsEmpty())
		{
			MessageText->SetText(NewLine);
		}
		else
		{
			MessageText->SetText(FText::Format(FText::FromString(TEXT("{0}\n{1}")), NewLine, Existing));
		}
		MessageText->SetColorAndOpacity(FSlateColor(Color));
	}
	SetVisibility(ESlateVisibility::Visible);

	// Reset timer: hide 3s after the LAST buff message
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
		World->GetTimerManager().SetTimer(HideTimer, this, &UBuffMessageUI::Hide, 3.0f, false);
	}
}

void UBuffMessageUI::ShowPersistent(const FString& Message, FColor Color)
{
	Show(Message, Color);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
	}
}

void UBuffMessageUI::Hide()
{
	if (MessageText)
	{
		MessageText->SetText(FText::GetEmpty());
	}
	SetVisibility(ESlateVisibility::Hidden);
}
