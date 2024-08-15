// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

class UButton;
class UMultiplayerSessionsSubsystem;
class FOnlineSessionSearchResult;

UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void MenuSetup(
        int32 NumberOfPublicConnections = 4, FString TypeOfMatch = "FreeForAll", FString LobbyPath = "/Game/ThirdPerson/Maps/Lobby");

protected:
    virtual bool Initialize() override;
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

    UFUNCTION()
    void HostButtonClicked();

    UFUNCTION()
    void JoinButtonClicked();

    void MenuTearDown();

    // the subsystem designed to handle all online session functionality
    UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

    int32 NumPublicConnections = 4;
    FString MatchType = "FreeForAll";
    FString PathToLobby = "";
};
