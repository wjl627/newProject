// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterPlayerController.generated.h"

class UInputMappingContext;
class AShooterCharacter;
class UShooterBulletCounterUI;
class UShooterUI;
class UBuffMessageUI;
enum class EMatchState : uint8;

/**
 *  第一人称射击游戏的简单 PlayerController
 *  管理输入映射
 *  当玩家 Pawn 被销毁时重生它
 */
UCLASS(abstract, config="Game")
class NEWPROJECT_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** 该玩家的输入映射上下文 */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** 输入映射上下文 */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** 要生成的移动端控制小部件 */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** 指向移动端控制小部件的指针 */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** 如果为 true，即使不在移动平台上游玩，玩家也会使用 UMG 触控 */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** 当被控制的 Pawn 被销毁时要重生的角色类 */
	UPROPERTY(EditAnywhere, Category="Shooter|Respawn")
	TSubclassOf<AShooterCharacter> CharacterClass;

	/** 要生成的子弹计数器 UI 小部件类型 */
	UPROPERTY(EditAnywhere, Category="Shooter|UI")
	TSubclassOf<UShooterBulletCounterUI> BulletCounterUIClass;

	/** 添加到被控制 Pawn 上以标记为玩家的标签 */
	UPROPERTY(EditAnywhere, Category="Shooter|Player")
	FName PlayerPawnTag = FName("Player");

	/** 要生成的记分板 UI 小部件类型 */
	UPROPERTY(EditAnywhere, Category="Shooter|UI")
	TSubclassOf<UShooterUI> ScoreboardUIClass;

	/** 指向子弹计数器 UI 小部件的指针 */
	UPROPERTY()
	TObjectPtr<UShooterBulletCounterUI> BulletCounterUI;

	/** 指向记分板 UI 小部件的指针 */
	UPROPERTY()
	TObjectPtr<UShooterUI> ScoreboardUI;

	/** 比赛是否已开始（大厅期间阻止 UI 更新） */
	bool bMatchStarted = false;


	/** 增益消息小部件类（蓝图子类） */
	UPROPERTY(EditAnywhere, Category="Shooter|UI")
	TSubclassOf<UBuffMessageUI> BuffMessageUIClass;

	/** 用于显示增益通知消息的小部件 */
	UPROPERTY()
	TObjectPtr<UBuffMessageUI> BuffMessageWidget;

	/** 主菜单小部件类（创建/加入会话） */
	UPROPERTY(EditAnywhere, Category="Shooter|UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	/** 大厅小部件类（玩家列表 + 开始按钮） */
	UPROPERTY(EditAnywhere, Category="Shooter|UI")
	TSubclassOf<UUserWidget> LobbyWidgetClass;

	/** 指向主菜单小部件的指针 */
	UPROPERTY()
	TObjectPtr<UUserWidget> MainMenuWidget;

	/** 指向大厅小部件的指针 */
	UPROPERTY()
	TObjectPtr<UUserWidget> LobbyWidget;

protected:

	/** 游戏逻辑初始化 */
	virtual void BeginPlay() override;

	/** 初始化输入绑定 */
	virtual void SetupInputComponent() override;

	/** Pawn 初始化（权威服务器 / 监听服务器主机） */
	virtual void OnPossess(APawn* InPawn) override;

	/** 拥有该 Pawn 的客户端上的 Pawn 初始化（专用客户端） */
	virtual void AcknowledgePossession(APawn* P) override;

	/** 为本地玩家的射击角色绑定 HUD 委托 */
	void SetupShooterCharacterUI(AShooterCharacter* ShooterCharacter);

	/** 当被控制的 Pawn 被销毁时调用 */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	/** 当被控制 Pawn 上的子弹数量更新时调用 */
	UFUNCTION()
	void OnBulletCountUpdated(int32 MagazineSize, int32 Bullets);

	/** 当被控制的 Pawn 受到伤害时调用 */
	UFUNCTION()
	void OnPawnDamaged(float LifePercent);

	/** 如果玩家应使用 UMG 触控则返回 true */
	bool ShouldUseTouchControls() const;

	/** 移除所有增强输入映射上下文（屏蔽游戏输入） */
	void BlockAllGameInput();

	/** 重新添加增强输入映射上下文（启用游戏输入） */
	void EnableGameInput();

public:
	bool bIsHost = false;
	void ShowMainMenu();
	void ShowLobby();

	/** 在该客户端上更新玩家分数的客户端 RPC */
	UFUNCTION(Client, Reliable)
	void Client_UpdatePlayerScore(int32 Score);

	/** 在该客户端上更新队伍分数的客户端 RPC */
	UFUNCTION(Client, Reliable)
	void Client_UpdateTeamScore(uint8 TeamByte, int32 Score);

	/** 显示增益通知消息的客户端 RPC */
	UFUNCTION(Client, Reliable)
	void Client_ShowBuffMessage(const FString& Message, FColor Color);

	/** 更新剩余比赛时间的客户端 RPC */
	UFUNCTION(Client, Reliable)
	void Client_UpdateMatchTime(int32 RemainingSeconds);

	/** 显示比赛排名的客户端 RPC（对所有玩家相同） */
	UFUNCTION(Client, Reliable)
	void Client_ShowMatchRanking(const FString& RankingText);

	/** 比赛状态变化时由 GameState 的 OnRep 调用（在每个客户端运行） */
	void OnMatchStateChanged(EMatchState NewState);

	/** 服务器 RPC：房主请求开始比赛 */
	UFUNCTION(Server, Reliable)
	void Server_StartMatch();

	/** 当房主在大厅按下 Enter 时调用 */
	void OnLobbyStartPressed();
};
