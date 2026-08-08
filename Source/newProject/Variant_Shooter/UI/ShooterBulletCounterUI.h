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
 *  子弹计数器和生命条 UI 小部件
 *  完全由 C++ 实现的小部件，无需蓝图子类
 */
UCLASS()
class NEWPROJECT_API UShooterBulletCounterUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 玩家受到伤害时显示的屏幕边缘边框 */
	UPROPERTY()
	TArray<TObjectPtr<UBorder>> DamageFrameBorders;

	/** 上一次已知的生命百分比，用于检测伤害事件 */
	float LastLifePercent = -1.0f;

	/** 伤害边框当前的不透明度（0 = 隐藏） */
	float DamageFlashAlpha = 0.0f;

	FTimerHandle DamageFlashTimer;

	void TickDamageFlash();

	/** 伤害闪烁淡出所需的时间 */
	static constexpr float DamageFlashFadeDuration = 0.45f;

	/** 黄色伤害边框的峰值不透明度 */
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

	/** 设置弹药显示 */
	void SetAmmo(int32 MagazineSize, int32 BulletCount);

	/** 设置生命条填充 */
	void SetHealthPercent(float LifePercent);

	/** 允许蓝图响应子弹数量变化（例如动画） */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "UpdateBulletCounter"))
	void BP_UpdateBulletCounter(int32 MagazineSize, int32 BulletCount);

	/** 允许蓝图响应生命值变化（例如受击闪烁） */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "Damaged"))
	void BP_Damaged(float LifePercent);
};
