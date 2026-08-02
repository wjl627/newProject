#include "MainMenuUI.h"
#include "ShooterGameInstance.h"
#include "ShooterPlayerController.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"

static const TCHAR* ShooterListenMapURL = TEXT("/Game/Variant_Shooter/Lvl_Shooter?listen");

void UMainMenuUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (UShooterGameInstance* GI = GetGameInstance<UShooterGameInstance>())
	{
		GI->OnSessionNamesFound.AddUObject(this, &UMainMenuUI::HandleSessionNamesFound);
		GI->OnSessionCreated.AddUObject(this, &UMainMenuUI::HandleSessionCreated);
		GI->OnSessionCreateFailed.AddUObject(this, &UMainMenuUI::HandleSessionCreateFailed);
		GI->OnNetworkFailure.AddUObject(this, &UMainMenuUI::HandleNetworkFailure);

		OnLocalAddressReady(GI->GetLocalHostAddress());
	}
}

void UMainMenuUI::NativeDestruct()
{
	if (UShooterGameInstance* GI = GetGameInstance<UShooterGameInstance>())
	{
		GI->OnSessionNamesFound.RemoveAll(this);
		GI->OnSessionCreated.RemoveAll(this);
		GI->OnSessionCreateFailed.RemoveAll(this);
		GI->OnNetworkFailure.RemoveAll(this);
	}

	Super::NativeDestruct();
}

FString UMainMenuUI::GetLocalHostAddress() const
{
	if (const UShooterGameInstance* GI = GetGameInstance<UShooterGameInstance>())
	{
		return GI->GetLocalHostAddress();
	}
	return FString::Printf(TEXT("127.0.0.1:%d"), UShooterGameInstance::DefaultListenPort);
}

bool UMainMenuUI::ParseLanAddress(const FString& InAddress, FString& OutHost, int32& OutPort) const
{
	const FString Trimmed = InAddress.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		return false;
	}

	if (Trimmed.Contains(TEXT(":")))
	{
		FString PortStr;
		if (!Trimmed.Split(TEXT(":"), &OutHost, &PortStr))
		{
			return false;
		}
		OutHost = OutHost.TrimStartAndEnd();
		OutPort = FCString::Atoi(*PortStr.TrimStartAndEnd());
		if (OutPort <= 0)
		{
			OutPort = UShooterGameInstance::DefaultListenPort;
		}
	}
	else
	{
		OutHost = Trimmed;
		OutPort = UShooterGameInstance::DefaultListenPort;
	}

	return !OutHost.IsEmpty();
}

bool UMainMenuUI::IsThisMachineAddress(const FString& Host) const
{
	if (Host.Equals(TEXT("127.0.0.1")) || Host.Equals(TEXT("localhost"), ESearchCase::IgnoreCase))
	{
		return true;
	}

	if (const UShooterGameInstance* GI = GetGameInstance<UShooterGameInstance>())
	{
		return Host.Equals(GI->GetLocalIPAddress(), ESearchCase::IgnoreCase);
	}

	return false;
}

FString UMainMenuUI::MakeConnectString(const FString& Host, int32 Port) const
{
	return FString::Printf(TEXT("%s:%d"), *Host, Port);
}

void UMainMenuUI::ConnectOrHost(const FString& IP)
{
	FString Host;
	int32 Port = UShooterGameInstance::DefaultListenPort;
	if (!ParseLanAddress(IP, Host, Port))
	{
		const FString Hint = GetLocalHostAddress();
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow,
			FString::Printf(TEXT("请两端都输入主机局域网地址，例如 %s"), *Hint));
		OnHostingFailed();
		return;
	}

	// Same IP on both PCs: host machine matches → open listen server; others join.
	if (IsThisMachineAddress(Host))
	{
		HostAtAddress(IP);
	}
	else
	{
		JoinAtAddress(IP);
	}
}

void UMainMenuUI::HostAtAddress(const FString& Address)
{
	if (!GetWorld())
	{
		return;
	}

	FString Host;
	int32 Port = UShooterGameInstance::DefaultListenPort;
	if (!ParseLanAddress(Address, Host, Port))
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow,
			FString::Printf(TEXT("开房请输入本机局域网地址，例如 %s"), *GetLocalHostAddress()));
		OnHostingFailed();
		return;
	}

	if (!IsThisMachineAddress(Host))
	{
		const FString Local = GetLocalHostAddress();
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red,
			FString::Printf(TEXT("开房地址必须是本机 IP。本机是 %s，你输入的是 %s"), *Local, *Host));
		OnHostingFailed();
		OnConnectionFailed(FString::Printf(TEXT("开房请输入本机地址 %s"), *Local));
		return;
	}

	OnHostingStarted();
	const FString HostAddress = MakeConnectString(Host, Port);
	GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Green,
		FString::Printf(TEXT("作为主机开启... 请另一端输入相同地址: %s"), *HostAddress));
	GetWorld()->ServerTravel(ShooterListenMapURL);
}

void UMainMenuUI::JoinAtAddress(const FString& Address)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	FString Host;
	int32 Port = UShooterGameInstance::DefaultListenPort;
	if (!ParseLanAddress(Address, Host, Port))
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow,
			TEXT("加入请输入主机局域网地址"));
		OnConnectionFailed(TEXT("请输入主机局域网地址"));
		return;
	}

	const FString ConnectString = MakeConnectString(Host, Port);
	OnConnectingStarted(ConnectString);
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

void UMainMenuUI::HandleNetworkFailure(const FString& ErrorMessage)
{
	OnConnectionFailed(ErrorMessage);
}
