#include "MainMenuUI.h"
#include "ShooterGameInstance.h"
#include "ShooterPlayerController.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

void UMainMenuUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (UShooterGameInstance* GI = GetGameInstance<UShooterGameInstance>())
	{
		GI->OnSessionNamesFound.AddUObject(this, &UMainMenuUI::HandleSessionNamesFound);
		GI->OnSessionCreated.AddUObject(this, &UMainMenuUI::HandleSessionCreated);
		GI->OnSessionCreateFailed.AddUObject(this, &UMainMenuUI::HandleSessionCreateFailed);
	}
}

void UMainMenuUI::NativeDestruct()
{
	if (UShooterGameInstance* GI = GetGameInstance<UShooterGameInstance>())
	{
		GI->OnSessionNamesFound.RemoveAll(this);
		GI->OnSessionCreated.RemoveAll(this);
		GI->OnSessionCreateFailed.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UMainMenuUI::ConnectOrHost(const FString& IP)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	if (IP.IsEmpty())
	{
		// Host: reload map as listen server
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("作为主机开启服务器..."));
		GetWorld()->ServerTravel(TEXT("Lvl_Shooter?listen"));
		return;
	}

	// Client: connect to host
	FString ConnectString = IP;
	if (!ConnectString.Contains(TEXT(":")))
	{
		ConnectString += TEXT(":7777");
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("正在连接 %s..."), *ConnectString));
	PC->ClientTravel(ConnectString, TRAVEL_Absolute);
}

void UMainMenuUI::ExitGame()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
	}
}

void UMainMenuUI::FindGames()
{
	UShooterGameInstance* GI = GetGameInstance<UShooterGameInstance>();
	if (!GI)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("正在搜索房间..."));
	OnSearchingStarted();
	GI->FindGames();
}

void UMainMenuUI::JoinGame(int32 Index)
{
	UShooterGameInstance* GI = GetGameInstance<UShooterGameInstance>();
	if (!GI)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("正在加入房间 [%d]..."), Index));
	GI->JoinFoundSession(Index);
}

void UMainMenuUI::HandleSessionNamesFound(const TArray<FString>& Names)
{
	if (Names.Num() > 0)
	{
		FString Msg = FString::Printf(TEXT("找到 %d 个房间"), Names.Num());
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);
		for (int32 i = 0; i < Names.Num(); ++i)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("  [%d] %s"), i, *Names[i]));
		}
		OnSearchResultsReady(Names);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("没有找到房间"));
		OnNoSessionsFound();
	}
}

void UMainMenuUI::HandleSessionCreated()
{
	if (AShooterPlayerController* PC = GetOwningPlayer<AShooterPlayerController>())
	{
		PC->ShowLobby();
	}
}

void UMainMenuUI::HandleSessionCreateFailed()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("创建房间失败"));
	OnHostingFailed();
}
