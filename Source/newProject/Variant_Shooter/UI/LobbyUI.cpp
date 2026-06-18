#include "LobbyUI.h"
#include "ShooterPlayerController.h"
#include "ShooterGameState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

void ULobbyUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (AShooterPlayerController* PC = GetOwningPlayer<AShooterPlayerController>())
	{
		SetHostControlsVisible(PC->bIsHost);
	}

	RefreshPlayerList();
}

void ULobbyUI::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshTimer += InDeltaTime;
	if (RefreshTimer >= 1.0f)
	{
		RefreshTimer = 0.0f;
		RefreshPlayerList();
	}
}

void ULobbyUI::StartGame()
{
	if (AShooterPlayerController* PC = GetOwningPlayer<AShooterPlayerController>())
	{
		PC->Server_StartMatch();
	}
}

void ULobbyUI::RefreshPlayerList()
{
	TArray<FString> PlayerNames;

	if (AGameStateBase* GS = GetWorld()->GetGameState())
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (PS)
			{
				PlayerNames.Add(PS->GetPlayerName());
			}
		}
	}

	if (PlayerNames != CachedPlayerNames)
	{
		CachedPlayerNames = PlayerNames;
		OnPlayerListUpdated(CachedPlayerNames);
	}
}
