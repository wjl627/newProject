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

	/** IP empty → host & enter lobby; IP filled → connect to that server */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void ConnectOrHost(const FString& IP);

	/** Quit the game */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void ExitGame();

	/** Search for LAN sessions (PIE use) */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void FindGames();

	/** Join a found session by index (PIE use) */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void JoinGame(int32 Index);

	/** Called when search results arrive */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnSearchResultsReady(const TArray<FString>& SessionNames);

	/** Called when hosting begins */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnHostingStarted();

	/** Called when hosting fails */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnHostingFailed();

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
};
