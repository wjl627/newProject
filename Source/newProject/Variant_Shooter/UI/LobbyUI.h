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

	/** 房主点击开始游戏 */
	UFUNCTION(BlueprintCallable, Category="Lobby")
	void StartGame();

	/** 当玩家列表变化时调用 */
	UFUNCTION(BlueprintImplementableEvent, Category="Lobby")
	void OnPlayerListUpdated(const TArray<FString>& PlayerNames);

	/** 用于显示/隐藏开始按钮（仅房主） */
	UFUNCTION(BlueprintImplementableEvent, Category="Lobby")
	void SetHostControlsVisible(bool bVisible);

protected:
	void RefreshPlayerList();

private:
	float RefreshTimer = 0.0f;
	TArray<FString> CachedPlayerNames;
};
