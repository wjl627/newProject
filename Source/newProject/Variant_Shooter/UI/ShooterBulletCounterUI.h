// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "ShooterBulletCounterUI.generated.h"

/**
 *  Bullet counter and health bar UI widget
 *  Self-contained C++ widget, no blueprint subclass required
 */
UCLASS()
class NEWPROJECT_API UShooterBulletCounterUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Screen-edge borders shown when the player takes damage */
	UPROPERTY()
	TArray<TObjectPtr<UBorder>> DamageFrameBorders;

	/** Last known life percent, used to detect damage events */
	float LastLifePercent = -1.0f;

	/** Current opacity of the damage frame (0 = hidden) */
	float DamageFlashAlpha = 0.0f;

	FTimerHandle DamageFlashTimer;

	void TickDamageFlash();

	/** How long the damage flash takes to fade out */
	static constexpr float DamageFlashFadeDuration = 0.45f;

	/** Peak opacity of the yellow damage frame */
	static constexpr float DamageFlashPeakAlpha = 0.65f;

	void CreateDamageFrame();
	void PlayDamageFlash();
	void UpdateDamageFrameOpacity(float Alpha);

public:

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> AmmoText;

	UPROPERTY()
	TObjectPtr<UBorder> ResultBackground;

	UPROPERTY()
	TObjectPtr<UTextBlock> ResultText;

	/** Sets the ammo display */
	void SetAmmo(int32 MagazineSize, int32 BulletCount);

	/** Sets the health bar fill */
	void SetHealthPercent(float LifePercent);

	/** Shows match result overlay */
	void ShowMatchResult(const FString& Message, FColor Color);

	/** Shows the match ranking (top 3) */
	void ShowMatchRanking(const FString& RankingText);

	/** Allows Blueprint to react to bullet count changes (e.g. animations) */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "UpdateBulletCounter"))
	void BP_UpdateBulletCounter(int32 MagazineSize, int32 BulletCount);

	/** Allows Blueprint to react to health changes (e.g. hit flash) */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "Damaged"))
	void BP_Damaged(float LifePercent);
};
