// Copyright Epic Games, Inc. All Rights Reserved.

#include "MatchResultOverlay.h"
#include "Blueprint/WidgetTree.h"

void UMatchResultOverlay::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	ResultTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultLabel"));
	if (ResultTextBlock)
	{
		WidgetTree->RootWidget = ResultTextBlock;

		FSlateFontInfo Font;
		Font.Size = 48;
		ResultTextBlock->SetFont(Font);
		ResultTextBlock->SetJustification(ETextJustify::Center);
		ResultTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));

		// Vertical alignment via padding: push down 35% from top
		ResultTextBlock->SetMargin(FMargin(20.0f, 200.0f, 20.0f, 0.0f));
	}
}

void UMatchResultOverlay::SetResult(const FString& Message, FColor Color)
{
	if (ResultTextBlock)
	{
		ResultTextBlock->SetText(FText::FromString(Message));
		ResultTextBlock->SetColorAndOpacity(FSlateColor(Color));
	}
}
