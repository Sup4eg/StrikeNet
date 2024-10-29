// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseWidget.generated.h"

class UButton;
class UMultiplayerSessionsSubsystem;
class APlayerController;
class USoundBase;

UCLASS()
class BLASTER_API UPauseWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void MenuSetup();
    void MenuTearDown();

    virtual bool Initialize() override;

protected:
    UFUNCTION()
    void OnDestroySession(bool bWasSuccessful);

    UFUNCTION()
    void OnPlayerLeftGame();

private:
    UFUNCTION()
    void ReturnButtonClicked();

    UFUNCTION()
    void SelfDestructionButtonClicked();

    UFUNCTION()
    void PlayerCharacterBeginPlay();

    bool IsBlasterPlayerControllerValid();

    UPROPERTY(meta = (BindWidget))
    UButton* ReturnButton;

    UPROPERTY(meta = (BindWidget))
    UButton* SelfDestructionButton;

    UPROPERTY()
    UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

    UPROPERTY()
    ABlasterPlayerController* BlasterPlayerController;

    UPROPERTY(EditDefaultsOnly)
    USoundBase* OpenSound;
};
