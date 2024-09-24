// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "LobbyGameMode.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (!GameState || !GetWorld()) return;
    int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
    if (NumberOfPlayers == AllowedNumberOfPlayers)
    {
        bUseSeamlessTravel = true;
        GetWorld()->ServerTravel("/Game/Maps/BlasterMap?listen");
    }
}
