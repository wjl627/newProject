// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "GameFramework/PlayerController.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "newProject.h"

static const FName SESSION_NAME = TEXT("ShooterSession");
static const FName SETTING_MAP = TEXT("MAP_NAME");
static const TCHAR* MainMenuMapURL = TEXT("/Game/FirstPerson/Lvl_FirstPerson");

void UShooterGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UShooterGameInstance::OnEngineNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UShooterGameInstance::OnEngineTravelFailure);
	}

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
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
	}
	Super::Shutdown();
}

FString UShooterGameInstance::GetLocalIPAddress() const
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return TEXT("127.0.0.1");
	}

	bool bCanBindAll = false;
	TSharedRef<FInternetAddr> LocalAddr = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBindAll);
	if (LocalAddr->IsValid())
	{
		return LocalAddr->ToString(false);
	}

	return TEXT("127.0.0.1");
}

FString UShooterGameInstance::GetLocalHostAddress() const
{
	return FString::Printf(TEXT("%s:%d"), *GetLocalIPAddress(), DefaultListenPort);
}

void UShooterGameInstance::OnEngineNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (World != GetWorld())
	{
		return;
	}

	const FString Message = ErrorString.IsEmpty()
		? FString::Printf(TEXT("网络错误: %s"), ENetworkFailure::ToString(FailureType))
		: ErrorString;
	NotifyNetworkFailure(Message);
}

void UShooterGameInstance::OnEngineTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (World != GetWorld())
	{
		return;
	}

	const FString Message = ErrorString.IsEmpty()
		? FString::Printf(TEXT("旅行失败: %s"), ETravelFailure::ToString(FailureType))
		: ErrorString;
	NotifyNetworkFailure(Message);
}

void UShooterGameInstance::NotifyNetworkFailure(const FString& ErrorString)
{
	UE_LOG(LognewProject, Error, TEXT("[GameInstance] %s"), *ErrorString);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red, ErrorString);
	}

	OnNetworkFailure.Broadcast(ErrorString);
	ReturnToMainMenuMap();
}

void UShooterGameInstance::ReturnToMainMenuMap()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FString CurrentMap = UGameplayStatics::GetCurrentLevelName(World, true);
	if (CurrentMap.Equals(TEXT("Lvl_FirstPerson"), ESearchCase::IgnoreCase)
		&& World->GetNetMode() == NM_Standalone)
	{
		return;
	}

	if (APlayerController* PC = GetFirstLocalPlayerController())
	{
		PC->ClientTravel(MainMenuMapURL, TRAVEL_Absolute);
	}
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
		NotifyNetworkFailure(FString::Printf(TEXT("加入房间失败: %d"), static_cast<int32>(Result)));
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
