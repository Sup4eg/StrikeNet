// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimmedWidget.generated.h"

class UTextBlock;

UCLASS()
class BLASTER_API UElimmedWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    UTextBlock* KillText;
};
