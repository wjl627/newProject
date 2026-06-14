// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "BuffMessageUI.generated.h"

UCLASS()
class NEWPROJECT_API UBuffMessageUI : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY()
	TObjectPtr<UTextBlock> MessageText;

	FTimerHandle HideTimer;

protected:
	virtual void NativeConstruct() override;

public:
	void Show(const FString& Message, FColor Color);
	void ShowPersistent(const FString& Message, FColor Color);
	void Hide();
};
