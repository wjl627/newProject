// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterBulletCounterUI.h"
#include "Blueprint/WidgetTree.h"
#include "Widgets/Layout/Anchors.h"
#include "TimerManager.h"
#include "Engine/World.h"

namespace ShooterBulletCounterUI_Private
{
	static UBorder* CreateDamageEdge(UWidgetTree* WidgetTree, UCanvasPanel* Canvas, const FAnchors& Anchors)
	{
		if (!WidgetTree || !Canvas)
		{
			return nullptr;
		}

		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		if (!Border)
		{
			return nullptr;
		}

		Border->SetBrushColor(FLinearColor(1.0f, 0.82f, 0.0f, 0.0f));
		Border->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Border))
		{
			PanelSlot->SetAnchors(Anchors);
			PanelSlot->SetOffsets(FMargin(0.0f));
			PanelSlot->SetAlignment(FVector2D::ZeroVector);
		}

		return Border;
	}
}

void UShooterBulletCounterUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		UE_LOG(LogTemp, Error, TEXT("[ShooterBulletCounterUI] WidgetTree is NULL!"));
		return;
	}

	// Create the canvas panel if not already present
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		if (RootCanvas)
		{
			WidgetTree->RootWidget = RootCanvas;
		}
	}

	if (RootCanvas)
	{
		CreateDamageFrame();

		// Create health bar
		if (!HealthBar)
		{
			HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
			if (HealthBar)
			{
				HealthBar->SetFillColorAndOpacity(FLinearColor::Green);
				HealthBar->SetPercent(1.0f);

				UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(HealthBar);
				if (PanelSlot)
				{
					PanelSlot->SetAnchors(FAnchors(0.02f, 0.93f, 0.15f, 0.98f));
					PanelSlot->SetAlignment(FVector2D(0.0f, 0.0f));
				}
			}
		}

		// Create ammo text
		if (!AmmoText)
		{
			AmmoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoText"));
			if (AmmoText)
			{
				AmmoText->SetText(FText::FromString(TEXT("10 / 10")));
				AmmoText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
				FSlateFontInfo AmmoFont;
				AmmoFont.Size = 24;
				AmmoText->SetFont(AmmoFont);
				AmmoText->SetJustification(ETextJustify::Right);

				UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(AmmoText);
				if (PanelSlot)
				{
					PanelSlot->SetAnchors(FAnchors(0.8f, 0.9f, 0.98f, 0.97f));
					PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
				}
			}
		}
	}
}

void UShooterBulletCounterUI::CreateDamageFrame()
{
	if (!RootCanvas || DamageFrameBorders.Num() > 0)
	{
		return;
	}

	constexpr float EdgeSize = 0.055f;

	DamageFrameBorders.Add(ShooterBulletCounterUI_Private::CreateDamageEdge(WidgetTree, RootCanvas, FAnchors(0.0f, 0.0f, 1.0f, EdgeSize)));
	DamageFrameBorders.Add(ShooterBulletCounterUI_Private::CreateDamageEdge(WidgetTree, RootCanvas, FAnchors(0.0f, 1.0f - EdgeSize, 1.0f, 1.0f)));
	DamageFrameBorders.Add(ShooterBulletCounterUI_Private::CreateDamageEdge(WidgetTree, RootCanvas, FAnchors(0.0f, 0.0f, EdgeSize, 1.0f)));
	DamageFrameBorders.Add(ShooterBulletCounterUI_Private::CreateDamageEdge(WidgetTree, RootCanvas, FAnchors(1.0f - EdgeSize, 0.0f, 1.0f, 1.0f)));

	DamageFrameBorders.RemoveAll([](const UBorder* Border) { return Border == nullptr; });
}

void UShooterBulletCounterUI::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DamageFlashTimer);
	}

	Super::NativeDestruct();
}

void UShooterBulletCounterUI::TickDamageFlash()
{
	if (UWorld* World = GetWorld())
	{
		const float DeltaTime = World->GetDeltaSeconds();
		DamageFlashAlpha = FMath::Max(0.0f, DamageFlashAlpha - (DeltaTime / DamageFlashFadeDuration));
		UpdateDamageFrameOpacity(DamageFlashAlpha);

		if (DamageFlashAlpha <= 0.0f)
		{
			World->GetTimerManager().ClearTimer(DamageFlashTimer);
		}
	}
}

void UShooterBulletCounterUI::UpdateDamageFrameOpacity(float Alpha)
{
	const FLinearColor FrameColor(1.0f, 0.82f, 0.0f, Alpha);
	for (UBorder* Border : DamageFrameBorders)
	{
		if (Border)
		{
			Border->SetBrushColor(FrameColor);
		}
	}
}

void UShooterBulletCounterUI::PlayDamageFlash()
{
	DamageFlashAlpha = DamageFlashPeakAlpha;
	UpdateDamageFrameOpacity(DamageFlashAlpha);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DamageFlashTimer);
		World->GetTimerManager().SetTimer(DamageFlashTimer, this, &UShooterBulletCounterUI::TickDamageFlash, 0.016f, true);
	}
}

void UShooterBulletCounterUI::SetAmmo(int32 MagazineSize, int32 BulletCount)
{
	if (AmmoText)
	{
		const FString AmmoString = FString::Printf(TEXT("%d / %d"), BulletCount, MagazineSize);
		AmmoText->SetText(FText::FromString(AmmoString));
	}
	BP_UpdateBulletCounter(MagazineSize, BulletCount);
}

void UShooterBulletCounterUI::SetHealthPercent(float LifePercent)
{
	const bool bTookDamage = LastLifePercent >= 0.0f && LifePercent < LastLifePercent - KINDA_SMALL_NUMBER;
	LastLifePercent = LifePercent;

	if (HealthBar)
	{
		HealthBar->SetPercent(LifePercent);
		if (LifePercent < 0.33f)
		{
			HealthBar->SetFillColorAndOpacity(FLinearColor::Red);
		}
		else if (LifePercent < 0.66f)
		{
			HealthBar->SetFillColorAndOpacity(FLinearColor::Yellow);
		}
		else
		{
			HealthBar->SetFillColorAndOpacity(FLinearColor::Green);
		}
	}

	if (bTookDamage)
	{
		PlayDamageFlash();
	}

	BP_Damaged(LifePercent);
}
