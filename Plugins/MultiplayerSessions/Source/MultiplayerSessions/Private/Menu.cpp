// Fill out your copyright notice in the Description page of Project Settings.

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Menu.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
    if (!GetWorld() || !GetWorld()->GetFirstPlayerController() || !GetGameInstance()) return;

    PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
    NumPublicConnections = NumberOfPublicConnections;
    MatchType = TypeOfMatch;

    AddToViewport();
    SetUpWidgetSettings();

    UGameInstance* GameInstance = GetGameInstance();
    MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
        MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
        MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
        MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestorySession);
        MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
    }
}

bool UMenu::Initialize()
{
    bool result = Super::Initialize();
    check(GEngine);
    check(GetWorld());
    if (HostButton && JoinButton)
    {
        HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
        JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
    }
    return result;
}

void UMenu::NativeDestruct()
{
    Super::NativeDestruct();
    MenuTearDown();
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, TEXT("Session created successfully"));
        GetWorld()->ServerTravel(PathToLobby);
        if (MultiplayerSessionsSubsystem)
        {
            MultiplayerSessionsSubsystem->StartSession();
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("Failed to create the session!"));
        HostButton->SetIsEnabled(true);
    }
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
    if (!MultiplayerSessionsSubsystem) return;
    for (auto Result : SessionResults)
    {
        FString SettingsValue;
        Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
        if (SettingsValue == MatchType)
        {
            MultiplayerSessionsSubsystem->JoinSession(Result);
            return;
        }
    }
    if (!bWasSuccessful || SessionResults.Num() <= 0)
    {
        JoinButton->SetIsEnabled(true);
    }
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSessionPtr SessionInterface = MultiplayerSessionsSubsystem->GetSessionInterface();
    if (SessionInterface)
    {
        FString Address;
        SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

        APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
        if (PlayerController)
        {
            PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
        }
    }
    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        JoinButton->SetIsEnabled(true);
    }
}

void UMenu::OnDestorySession(bool bWasSuccessful)
{
    GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Black, FString::Printf(TEXT("Destroyed Session Successfully Menu call")));
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Black, FString::Printf(TEXT("Started Session Successfully Menu call")));
    }
}

void UMenu::SetUpWidgetSettings()
{
    SetVisibility(ESlateVisibility::Visible);
    SetIsFocusable(true);

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    FInputModeUIOnly InputModeData;
    InputModeData.SetWidgetToFocus(TakeWidget());
    InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputModeData);
    PlayerController->bShowMouseCursor = true;
}

void UMenu::HostButtonClicked()
{
    if (!MultiplayerSessionsSubsystem) return;
    HostButton->SetIsEnabled(false);
    MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
}

void UMenu::JoinButtonClicked()
{
    if (!MultiplayerSessionsSubsystem) return;
    JoinButton->SetIsEnabled(false);
    MultiplayerSessionsSubsystem->FindSessions(10000);
}

void UMenu::MenuTearDown()
{
    if (!GetWorld()->GetFirstPlayerController()) return;
    RemoveFromParent();
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController)
    {
        FInputModeGameOnly InputModeData;
        PlayerController->SetInputMode(InputModeData);
        PlayerController->SetShowMouseCursor(false);
    }
}
