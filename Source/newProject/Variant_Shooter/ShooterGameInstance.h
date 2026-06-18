// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ShooterGameInstance.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSessionsFound, const TArray<FOnlineSessionSearchResult>&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSessionNamesFound, const TArray<FString>&);

UCLASS()
class NEWPROJECT_API UShooterGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/** Host a new game session */
	void HostGame(int32 MaxPlayers);

	/** Search for available sessions */
	void FindGames();

	/** Join a found session */
	void JoinGame(const FOnlineSessionSearchResult& Result);

	/** Destroy the current session */
	void DestroySession();

	/** Delegates for UI notification */
	FOnSessionsFound OnSessionsFound;

	/** Session names found (safe for UMG widgets, no Online types) */
	FOnSessionNamesFound OnSessionNamesFound;

	/** Returns the number of found sessions */
	int32 GetFoundSessionCount() const;

	/** Joins a found session by index */
	void JoinFoundSession(int32 Index);

	/** Called when a game is found, for simple auto-join */
	FSimpleMulticastDelegate OnGameJoined;

	/** Called when a session is successfully created */
	FSimpleMulticastDelegate OnSessionCreated;

	/** Called when session creation fails */
	FSimpleMulticastDelegate OnSessionCreateFailed;

private:
	IOnlineSessionPtr SessionInterface;

	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;

	FDelegateHandle OnCreateSessionCompleteHandle;
	FDelegateHandle OnFindSessionsCompleteHandle;
	FDelegateHandle OnJoinSessionCompleteHandle;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};
