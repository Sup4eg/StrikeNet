// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "MultiplayerSessionsSubsystem.h"
#include "LobbyGameMode.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (!GameState || !GetWorld()) return;
    int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
        check(Subsystem);
        if (NumberOfPlayers == Subsystem->DesiredNumPublicConnections)
        {
            bUseSeamlessTravel = true;
            FString MatchType = Subsystem->DesiredMatchType;
            if (MatchType == "DeathMatch")
            {
                GetWorld()->ServerTravel("/Game/Maps/DeathMatchMap?listen");
            }
            else if (MatchType == "Teams")
            {
                GetWorld()->ServerTravel("/Game/Maps/TeamsMap?listen");
            }
            else if (MatchType == "CTF")
            {
                GetWorld()->ServerTravel("/Game/Maps/CTFMap?listen");
            }
        }
    }
}
