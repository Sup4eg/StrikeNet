// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncementWidget.generated.h"

class UHorizontalBox;
class UTextBlock;

UCLASS()
class BLASTER_API UElimAnnouncementWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetElimAnnouncementText(FString AttackerName, FString VictimName);

    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* AnnouncementBox;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* KillerNameText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* VictimNameText;
};
