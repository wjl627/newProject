#include "ShooterGameState.h"
#include "Net/UnrealNetwork.h"
#include "ShooterPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void AShooterGameState::SetMatchState(EMatchState NewState)
{
	if (HasAuthority() && MatchState != NewState)
	{
		MatchState = NewState;
		OnRep_MatchState();
	}
}

void AShooterGameState::SetRemainingTime(float NewTime)
{
	if (HasAuthority())
	{
		RemainingTime = NewTime;
	}
}

void AShooterGameState::OnRep_MatchState()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AShooterPlayerController* SPC = Cast<AShooterPlayerController>(It->Get()))
		{
			SPC->OnMatchStateChanged(MatchState);
		}
	}
}

void AShooterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShooterGameState, MatchState);
	DOREPLIFETIME(AShooterGameState, RemainingTime);
}
