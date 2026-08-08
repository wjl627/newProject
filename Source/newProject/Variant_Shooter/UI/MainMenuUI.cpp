#include "MainMenuUI.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

static const TCHAR* ShooterMapPath = TEXT("/Game/Variant_Shooter/Lvl_Shooter");
static const TCHAR* DefaultPort = TEXT(":7777");

bool UMainMenuUI::IsOwnLanAddress(const FString& Host)
{
	// 空地址表示“仅作为主机”。有意排除回环地址，因此输入
	// 127.0.0.1 会作为客户端加入本机，这是同一台电脑上两个实例连接的方式。
	if (Host.IsEmpty())
	{
		return true;
	}

	if (Host.Equals(TEXT("localhost"), ESearchCase::IgnoreCase) || Host == TEXT("127.0.0.1"))
	{
		return false;
	}

	ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!Sockets)
	{
		return false;
	}

	TArray<TSharedPtr<FInternetAddr>> Adapters;
	if (Sockets->GetLocalAdapterAddresses(Adapters))
	{
		for (const TSharedPtr<FInternetAddr>& Addr : Adapters)
		{
			// ToString(false) 会省略端口，留下裸地址用于比较
			if (Addr.IsValid() && Addr->ToString(false) == Host)
			{
				return true;
			}
		}
	}

	return false;
}

FString UMainMenuUI::GetLocalIP() const
{
	ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!Sockets)
	{
		return TEXT("127.0.0.1");
	}

	bool bCanBindAll = false;
	const TSharedRef<FInternetAddr> Addr = Sockets->GetLocalHostAddr(*GLog, bCanBindAll);
	return Addr->ToString(false);
}

void UMainMenuUI::ConnectOrHost(const FString& IP)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	// 去掉可选的 ":port" 端口，以便用裸地址与本地网卡地址匹配
	const FString Trimmed = IP.TrimStartAndEnd();
	FString Host = Trimmed;
	FString Port;
	Trimmed.Split(TEXT(":"), &Host, &Port);

	if (IsOwnLanAddress(Host))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("作为主机开启服务器 (%s)..."), *GetLocalIP()));

		if (GetWorld()->ServerTravel(FString::Printf(TEXT("%s?listen"), ShooterMapPath)))
		{
			OnHostingStarted();
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("开启服务器失败"));
			OnHostingFailed();
		}
		return;
	}

	const FString ConnectString = Port.IsEmpty() ? Host + DefaultPort : Trimmed;

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
