// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterPlayerController.generated.h"

class UInputMappingContext;
class AShooterCharacter;
class UShooterBulletCounterUI;
class UShooterUI;
class UBuffMessageUI;

/**
 *  Simple PlayerController for a first person shooter game
 *  Manages input mappings
 *  Respawns the player pawn when it's destroyed
 */
UCLASS(abstract, config="Game")
class NEWPROJECT_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input mapping contexts for this player */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Character class to respawn when the possessed pawn is destroyed */
	UPROPERTY(EditAnywhere, Category="Shooter|Respawn")
	TSubclassOf<AShooterCharacter> CharacterClass;

	/** Type of bullet counter UI widget to spawn */
	UPROPERTY(EditAnywhere, Category="Shooter|UI")
	TSubclassOf<UShooterBulletCounterUI> BulletCounterUIClass;

	/** Tag to grant the possessed pawn to flag it as the player */
	UPROPERTY(EditAnywhere, Category="Shooter|Player")
	FName PlayerPawnTag = FName("Player");

	/** Type of scoreboard UI widget to spawn */
	UPROPERTY(EditAnywhere, Category="Shooter|UI")
	TSubclassOf<UShooterUI> ScoreboardUIClass;

	/** Pointer to the bullet counter UI widget */
	UPROPERTY()
	TObjectPtr<UShooterBulletCounterUI> BulletCounterUI;

	/** Pointer to the scoreboard UI widget */
	UPROPERTY()
	TObjectPtr<UShooterUI> ScoreboardUI;

	/** Widget for showing buff notification messages */
	UPROPERTY()
	TObjectPtr<UBuffMessageUI> BuffMessageWidget;

protected:

	/** Gameplay Initialization */
	virtual void BeginPlay() override;

	/** Initialize input bindings */
	virtual void SetupInputComponent() override;

	/** Pawn initialization (authority / listen-server host) */
	virtual void OnPossess(APawn* InPawn) override;

	/** Pawn initialization on the owning client (dedicated client) */
	virtual void AcknowledgePossession(APawn* P) override;

	/** Binds HUD delegates for the local player's shooter character */
	void SetupShooterCharacterUI(AShooterCharacter* ShooterCharacter);

	/** Called if the possessed pawn is destroyed */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	/** Called when the bullet count on the possessed pawn is updated */
	UFUNCTION()
	void OnBulletCountUpdated(int32 MagazineSize, int32 Bullets);

	/** Called when the possessed pawn is damaged */
	UFUNCTION()
	void OnPawnDamaged(float LifePercent);

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

public:

	/** Client RPC to update the player's score on this client */
	UFUNCTION(Client, Reliable)
	void Client_UpdatePlayerScore(int32 Score);

	/** Client RPC to update a team's score on this client */
	UFUNCTION(Client, Reliable)
	void Client_UpdateTeamScore(uint8 TeamByte, int32 Score);

	/** Client RPC to show a buff notification message */
	UFUNCTION(Client, Reliable)
	void Client_ShowBuffMessage(const FString& Message, FColor Color);

	/** Client RPC to update the remaining match time */
	UFUNCTION(Client, Reliable)
	void Client_UpdateMatchTime(int32 RemainingSeconds);

	/** Client RPC to announce the match result */
	UFUNCTION(Client, Reliable)
	void Client_AnnounceMatchResult(bool bIsWinner, const FString& WinnerName, int32 WinnerScore, int32 MyScore);

	/** Client RPC to show the match ranking (same for all players) */
	UFUNCTION(Client, Reliable)
	void Client_ShowMatchRanking(const FString& RankingText);
};
