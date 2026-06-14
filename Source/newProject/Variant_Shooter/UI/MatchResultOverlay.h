// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "MatchResultOverlay.generated.h"

UCLASS()
class NEWPROJECT_API UMatchResultOverlay : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UTextBlock> ResultTextBlock;

protected:
	virtual void NativeConstruct() override;

public:
	void SetResult(const FString& Message, FColor Color);
};
