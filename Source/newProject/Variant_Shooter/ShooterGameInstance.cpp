// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "newProject.h"

static const FName SESSION_NAME = TEXT("ShooterSession");
static const FName SETTING_MAP = TEXT("MAP_NAME");

void UShooterGameInstance::Init()
{
	Super::Init();

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UShooterGameInstance::OnCreateSessionComplete);
			OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UShooterGameInstance::OnFindSessionsComplete);
			OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UShooterGameInstance::OnJoinSessionComplete);

			UE_LOG(LognewProject, Log, TEXT("[GameInstance] OnlineSubsystem ready: %s"), *Subsystem->GetSubsystemName().ToString());
		}
	}
}

void UShooterGameInstance::Shutdown()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
	}
	Super::Shutdown();
}

void UShooterGameInstance::HostGame(int32 MaxPlayers)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LognewProject, Error, TEXT("[GameInstance] No session interface"));
		return;
	}

	// Destroy existing session first
	if (SessionInterface->GetNamedSession(SESSION_NAME))
	{
		SessionInterface->DestroySession(SESSION_NAME);
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = true;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = false;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.NumPublicConnections = MaxPlayers;
	SessionSettings.bUseLobbiesIfAvailable = false;
	SessionSettings.Set(SETTING_MAP, FString("Lvl_Shooter"), EOnlineDataAdvertisementType::ViaOnlineService);

	OnCreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), SESSION_NAME, SessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
		UE_LOG(LognewProject, Error, TEXT("[GameInstance] CreateSession failed"));
	}
}

void UShooterGameInstance::FindGames()
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = true;
	SessionSearch->MaxSearchResults = 10;

	OnFindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
	}
}

void UShooterGameInstance::JoinGame(const FOnlineSessionSearchResult& Result)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	OnJoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), SESSION_NAME, Result))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
	}
}

void UShooterGameInstance::DestroySession()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->DestroySession(SESSION_NAME);
	}
}

int32 UShooterGameInstance::GetFoundSessionCount() const
{
	return SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0;
}

void UShooterGameInstance::JoinFoundSession(int32 Index)
{
	if (SessionSearch.IsValid() && SessionSearch->SearchResults.IsValidIndex(Index))
	{
		JoinGame(SessionSearch->SearchResults[Index]);
	}
}

void UShooterGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
	}

	if (bWasSuccessful)
	{
		UE_LOG(LognewProject, Log, TEXT("[GameInstance] Session created successfully"));
		OnSessionCreated.Broadcast();
	}
	else
	{
		UE_LOG(LognewProject, Error, TEXT("[GameInstance] Session creation failed"));
		OnSessionCreateFailed.Broadcast();
	}
}

void UShooterGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
	}

	if (bWasSuccessful && SessionSearch.IsValid())
	{
		UE_LOG(LognewProject, Log, TEXT("[GameInstance] Found %d sessions"), SessionSearch->SearchResults.Num());
		OnSessionsFound.Broadcast(SessionSearch->SearchResults);

		TArray<FString> Names;
		for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
		{
			Names.Add(FString::Printf(TEXT("Session %d"), Names.Num() + 1));
		}
		OnSessionNamesFound.Broadcast(Names);
	}
	else
	{
		UE_LOG(LognewProject, Warning, TEXT("[GameInstance] Session search failed or no results"));
		OnSessionNamesFound.Broadcast(TArray<FString>());
	}
}

void UShooterGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LognewProject, Error, TEXT("[GameInstance] Join session failed: %d"), (int32)Result);
		return;
	}

	FString ConnectString;
	if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LognewProject, Log, TEXT("[GameInstance] Joining at: %s"), *ConnectString);

		if (APlayerController* PC = GetFirstLocalPlayerController())
		{
			PC->ClientTravel(ConnectString, TRAVEL_Absolute);
			OnGameJoined.Broadcast();
		}
	}
}
