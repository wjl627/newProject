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

	// 自动从用户蓝图查找第一个 TextBlock（任意名称均可）
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
		// 将新消息追加到顶部，保留旧消息在下方
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

	// 重置计时器：在最后一条增益消息后 3 秒隐藏
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
