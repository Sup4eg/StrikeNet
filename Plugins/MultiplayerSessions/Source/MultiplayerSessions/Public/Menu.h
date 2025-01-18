// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

class UButton;
class UCheckBox;
class UMultiplayerSessionsSubsystem;
class FOnlineSessionSearchResult;
class USettingsMenu;

UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void MenuSetup(int32 NumberOfPublicConnections = 4, FString LobbyPath = "/Game/ThirdPerson/Maps/Lobby");

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /*
        Callbacks for the custom delegates on the MultiplayerSessionsSubsystem
    */
    UFUNCTION()
    void OnCreateSession(bool bWasSuccessful);
    void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
    void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
    UFUNCTION()
    void OnDestorySession(bool bWasSuccessful);
    UFUNCTION()
    void OnStartSession(bool bWasSuccessful);

private:
    void SetUpWidgetSettings();

    UPROPERTY(meta = (BindWidget))
    UButton* HostButton;

    UPROPERTY(meta = (BindWidget))
    UButton* JoinButton;

    UPROPERTY(meta = (BindWidget))
    UButton* SettingsButton;

    UPROPERTY(meta = (BindWidget))
    UCheckBox* DeathMatchCheckBox;

    UPROPERTY(meta = (BindWidget))
    UCheckBox* TeamsCheckBox;

    UPROPERTY(meta = (BindWidget))
    UCheckBox* CTFCheckBox;

    UFUNCTION()
    void HostButtonClicked();

    UFUNCTION()
    void JoinButtonClicked();

    UFUNCTION()
    void SettingsButtonClicked();

    UFUNCTION()
    void DeathMatchCheckBoxClicked(bool bIsChecked);

    UFUNCTION()
    void TeamsCheckBoxClicked(bool bIsChecked);

    UFUNCTION()
    void CTFCheckBoxClicked(bool bIsChecked);

    void MenuTearDown();

    // the subsystem designed to handle all online session functionality
    UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

    UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    int32 NumPublicConnections = 10;
    
    FString MatchType = "DeathMatch";
    FString PathToLobby = "";

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<USettingsMenu> SettingsMenuClass;
};
