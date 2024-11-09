// Fill out your copyright notice in the Description page of Project Settings.

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Engine/GameInstance.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "SettingsMenu.h"
#include "Menu.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString LobbyPath)
{
    if (!GetWorld() || !GetWorld()->GetFirstPlayerController() || !GetGameInstance()) return;

    PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
    NumPublicConnections = NumberOfPublicConnections;

    AddToViewport();
    SetUpWidgetSettings();

    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    }
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
        MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
        MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
        MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestorySession);
        MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
    }
}

void UMenu::NativeConstruct()
{
    Super::NativeConstruct();
    if (HostButton && JoinButton && SettingsButton)
    {
        HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
        JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
        SettingsButton->OnClicked.AddDynamic(this, &ThisClass::SettingsButtonClicked);
    }

    // CheckBoxes
    if (DeathMatchCheckBox && TeamsCheckBox && CTFCheckBox)
    {
        DeathMatchCheckBox->OnCheckStateChanged.AddDynamic(this, &ThisClass::DeathMatchCheckBoxClicked);
        TeamsCheckBox->OnCheckStateChanged.AddDynamic(this, &ThisClass::TeamsCheckBoxClicked);
        CTFCheckBox->OnCheckStateChanged.AddDynamic(this, &ThisClass::CTFCheckBoxClicked);
    }

    if (DeathMatchCheckBox && DeathMatchCheckBox->OnCheckStateChanged.IsBound())
    {
        DeathMatchCheckBox->SetIsChecked(true);
        DeathMatchCheckBox->OnCheckStateChanged.Broadcast(true);
    }
}

void UMenu::NativeDestruct()
{
    Super::NativeDestruct();
    MenuTearDown();
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
    if (!GEngine) return;
    if (bWasSuccessful)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, TEXT("Session created successfully"));
        if (GetWorld())
        {
            GetWorld()->ServerTravel(PathToLobby);
        }
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
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Black, FString::Printf(TEXT("Destroyed Session Successfully Menu call")));
    }
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
    if (bWasSuccessful && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Black, FString::Printf(TEXT("Started Session Successfully Menu call")));
    }
}

void UMenu::SetUpWidgetSettings()
{
    SetVisibility(ESlateVisibility::Visible);
    SetIsFocusable(true);
    if (!GetWorld()) return;

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

void UMenu::SettingsButtonClicked()
{
    if (!SettingsMenuClass || !GetWorld()) return;
    SetVisibility(ESlateVisibility::Collapsed);
    if (USettingsMenu* SettingsMenu = CreateWidget<USettingsMenu>(GetWorld(), SettingsMenuClass))
    {
        SettingsMenu->AddToViewport();
    }
}

void UMenu::DeathMatchCheckBoxClicked(bool bIsChecked)
{
    if (bIsChecked)
    {
        MatchType = "DeathMatch";
        if (TeamsCheckBox)
        {
            TeamsCheckBox->SetIsChecked(false);
        }
        if (CTFCheckBox)
        {
            CTFCheckBox->SetIsChecked(false);
        }
    }
}

void UMenu::TeamsCheckBoxClicked(bool bIsChecked)
{
    if (bIsChecked)
    {
        MatchType = "Teams";
        if (DeathMatchCheckBox)
        {
            DeathMatchCheckBox->SetIsChecked(false);
        }
        if (CTFCheckBox)
        {
            CTFCheckBox->SetIsChecked(false);
        }
    }
}

void UMenu::CTFCheckBoxClicked(bool bIsChecked)
{
    if (bIsChecked)
    {
        MatchType = "CTF";
        if (DeathMatchCheckBox)
        {
            DeathMatchCheckBox->SetIsChecked(false);
        }
        if (TeamsCheckBox)
        {
            TeamsCheckBox->SetIsChecked(false);
        }
    }
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
