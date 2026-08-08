// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterUI.generated.h"

/**
 *  第一人称射击游戏的简单记分板 UI
 */
UCLASS(abstract)
class NEWPROJECT_API UShooterUI : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** 允许蓝图更新分数子小部件 */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Update Score"))
	void BP_UpdateScore(uint8 TeamByte, int32 Score);

	/** 允许蓝图更新玩家的个人分数 */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Update Player Score"))
	void BP_UpdatePlayerScore(int32 Score);

	/** 允许蓝图仅向该玩家显示增益通知 */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Show Buff Message"))
	void BP_ShowBuffMessage(const FString& Message, FColor Color);

	/** 允许蓝图更新剩余比赛时间显示 */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Update Match Time"))
	void BP_UpdateMatchTime(int32 RemainingSeconds);

	/** 允许蓝图显示比赛结果 */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Announce Match Result"))
	void BP_AnnounceMatchResult(bool bIsWinner, const FString& WinnerName, int32 WinnerScore, int32 MyScore);

	/** 允许蓝图显示比赛排名（前几名玩家） */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Show Match Ranking"))
	void BP_ShowMatchRanking(const FString& RankingText);
};
