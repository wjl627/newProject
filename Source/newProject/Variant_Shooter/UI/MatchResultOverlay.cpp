// Copyright Epic Games, Inc. All Rights Reserved.

#include "MatchResultOverlay.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Widgets/Layout/Anchors.h"

void UMatchResultOverlay::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	// 用于分层的根画布
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	if (!Canvas)
	{
		return;
	}
	WidgetTree->RootWidget = Canvas;

	// 深色背景遮罩
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	if (Background)
	{
		Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));
		UCanvasPanelSlot* BgSlot = Canvas->AddChildToCanvas(Background);
		if (BgSlot)
		{
			BgSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			BgSlot->SetOffsets(FMargin(0.0f));
			BgSlot->SetAlignment(FVector2D::ZeroVector);
		}
	}

	// 排名文本
	ResultTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultLabel"));
	if (ResultTextBlock)
	{
		FSlateFontInfo Font;
		Font.Size = 30;
		ResultTextBlock->SetFont(Font);
		ResultTextBlock->SetJustification(ETextJustify::Center);
		ResultTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));

		UCanvasPanelSlot* TextSlot = Canvas->AddChildToCanvas(ResultTextBlock);
		if (TextSlot)
		{
			TextSlot->SetAnchors(FAnchors(0.05f, 0.15f, 0.95f, 0.85f));
			TextSlot->SetOffsets(FMargin(0.0f));
			TextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		}
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
