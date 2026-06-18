// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterUI.generated.h"

/**
 *  Simple scoreboard UI for a first person shooter game
 */
UCLASS(abstract)
class NEWPROJECT_API UShooterUI : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Allows Blueprint to update score sub-widgets */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Update Score"))
	void BP_UpdateScore(uint8 TeamByte, int32 Score);

	/** Allows Blueprint to update the player's individual score */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Update Player Score"))
	void BP_UpdatePlayerScore(int32 Score);

	/** Allows Blueprint to show a buff notification to this player only */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Show Buff Message"))
	void BP_ShowBuffMessage(const FString& Message, FColor Color);

	/** Allows Blueprint to update the remaining match time display */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Update Match Time"))
	void BP_UpdateMatchTime(int32 RemainingSeconds);

	/** Allows Blueprint to display the match result */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Announce Match Result"))
	void BP_AnnounceMatchResult(bool bIsWinner, const FString& WinnerName, int32 WinnerScore, int32 MyScore);

	/** Allows Blueprint to display the match ranking (top players) */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Show Match Ranking"))
	void BP_ShowMatchRanking(const FString& RankingText);
};
