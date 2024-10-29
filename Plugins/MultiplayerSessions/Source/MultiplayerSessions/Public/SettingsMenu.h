// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsMenu.generated.h"

class UButton;
class UBaseSettingsTab;

UCLASS()
class MULTIPLAYERSESSIONS_API USettingsMenu : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual bool Initialize() override;

private:
    UFUNCTION()
    void GeneralButtonClicked();

    UFUNCTION()
    void GraphycsButtonClicked();

    UFUNCTION()
    void ApplyButtonClicked();

    UPROPERTY(meta = (BindWidget))
    UButton* GeneralButton;

    UPROPERTY(meta = (BindWidget))
    UButton* GraphycsButton;

    UPROPERTY(meta = (BindWidget))
    UButton* ApplyButton;

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UBaseSettingsTab> GeneralSettingsTabClass;

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UBaseSettingsTab> GraphycsSettingsTabClass;

    UPROPERTY()
    UBaseSettingsTab* GeneralSettingsTab;

    UPROPERTY()
    UBaseSettingsTab* GraphycsSettingsTab;
};
