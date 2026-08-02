#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuUI.generated.h"

UCLASS(BlueprintType, Blueprintable)
class NEWPROJECT_API UMainMenuUI : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 * Both PCs type the same host LAN IP (e.g. 192.168.1.10).
	 * If it matches this machine → host; otherwise → join.
	 * Prefer HostAtAddress / JoinAtAddress for same-PC PIE testing.
	 */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void ConnectOrHost(const FString& IP);

	/** Host: both sides type this machine's LAN IP, then host presses this */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void HostAtAddress(const FString& Address);

	/** Join: type the same LAN IP as the host, then join */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void JoinAtAddress(const FString& Address);

	/** Quit the game */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void ExitGame();

	/** Local LAN address for both sides to type, e.g. 192.168.1.10:7777 */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	FString GetLocalHostAddress() const;

	/** Search for LAN sessions (PIE use) */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void FindGames();

	/** Join a found session by index (PIE use) */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void JoinGame(int32 Index);

	/** Fired on construct with this machine's LAN host address */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnLocalAddressReady(const FString& Address);

	/** Called when search results arrive */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnSearchResultsReady(const TArray<FString>& SessionNames);

	/** Called when hosting begins */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnHostingStarted();

	/** Called when hosting fails */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnHostingFailed();

	/** Called when client starts connecting by IP */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnConnectingStarted(const FString& Address);

	/** Called when join / network travel fails */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnConnectionFailed(const FString& ErrorMessage);

	/** Called when searching begins */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnSearchingStarted();

	/** Called when no sessions were found */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnNoSessionsFound();

private:
	UFUNCTION()
	void HandleSessionNamesFound(const TArray<FString>& Names);

	void HandleSessionCreated();
	void HandleSessionCreateFailed();
	void HandleNetworkFailure(const FString& ErrorMessage);

	bool ParseLanAddress(const FString& InAddress, FString& OutHost, int32& OutPort) const;
	bool IsThisMachineAddress(const FString& Host) const;
	FString MakeConnectString(const FString& Host, int32 Port) const;
};
