#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyUI.generated.h"

UCLASS(BlueprintType, Blueprintable)
class NEWPROJECT_API ULobbyUI : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Host clicks Start Game */
	UFUNCTION(BlueprintCallable, Category="Lobby")
	void StartGame();

	/** Called when the player list changes */
	UFUNCTION(BlueprintImplementableEvent, Category="Lobby")
	void OnPlayerListUpdated(const TArray<FString>& PlayerNames);

	/** Called to show/hide the Start button (host only) */
	UFUNCTION(BlueprintImplementableEvent, Category="Lobby")
	void SetHostControlsVisible(bool bVisible);

	/** Host-only: LAN address clients should enter, e.g. 192.168.1.10:7777 */
	UFUNCTION(BlueprintImplementableEvent, Category="Lobby")
	void OnHostAddressReady(const FString& Address);

protected:
	void RefreshPlayerList();

private:
	float RefreshTimer = 0.0f;
	TArray<FString> CachedPlayerNames;
};
