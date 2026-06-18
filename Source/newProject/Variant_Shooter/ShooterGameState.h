#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ShooterGameState.generated.h"

UENUM(BlueprintType)
enum class EMatchState : uint8
{
	Lobby       UMETA(DisplayName = "Lobby"),
	InProgress  UMETA(DisplayName = "In Progress"),
	Ended       UMETA(DisplayName = "Ended")
};

UCLASS()
class NEWPROJECT_API AShooterGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	void SetMatchState(EMatchState NewState);
	EMatchState GetMatchState() const { return MatchState; }

	void SetRemainingTime(float NewTime);
	float GetRemainingTime() const { return RemainingTime; }

protected:
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	EMatchState MatchState = EMatchState::Lobby;

	UPROPERTY(Replicated)
	float RemainingTime = 0.0f;

	UFUNCTION()
	void OnRep_MatchState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
