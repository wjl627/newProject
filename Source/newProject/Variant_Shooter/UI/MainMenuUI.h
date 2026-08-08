#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuUI.generated.h"

UCLASS(BlueprintType, Blueprintable)
class NEWPROJECT_API UMainMenuUI : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 *  直接 IP 连接。双方输入相同地址——主机的局域网 IP。
	 *  如果该地址是本机自己的网卡地址（或留空），此实例
	 *  将作为监听服务器主机；其他任何地址都作为客户端加入。
	 *  输入 127.0.0.1 可强制客户端连接到本机（同一台电脑运行两个实例）。
	 */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void ConnectOrHost(const FString& IP);

	/** 本机的局域网 IP，方便主机从屏幕读取并分享 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="MainMenu")
	FString GetLocalIP() const;

	/** 退出游戏 */
	UFUNCTION(BlueprintCallable, Category="MainMenu")
	void ExitGame();

	/** 当此实例开始作为主机时调用 */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnHostingStarted();

	/** 当启动主机的地图跳转被拒绝时调用 */
	UFUNCTION(BlueprintImplementableEvent, Category="MainMenu")
	void OnHostingFailed();

private:
	/** 如果 Host 是本机自己的局域网地址或为空则返回 true */
	static bool IsOwnLanAddress(const FString& Host);
};
