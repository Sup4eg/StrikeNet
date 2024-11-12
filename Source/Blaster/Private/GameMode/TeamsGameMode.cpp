// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterGameState.h"
#include "BlasterPlayerState.h"
#include "BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "BlasterCharacter.h"
#include "TeamPlayerStart.h"
#include "TeamsGameMode.h"

ATeamsGameMode::ATeamsGameMode()
{
    bTeamsMatch = true;
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!NewPlayer || !NewPlayer->GetPawn()) return;
    if (Super::ShouldSpawnAtStartSpot(NewPlayer) && NewPlayer->StartSpot.IsValid())
    {
        AActor* StartSpot = NewPlayer->StartSpot.Get();
        NewPlayer->GetPawn()->SetActorLocation(StartSpot->GetActorLocation());
        NewPlayer->GetPawn()->SetActorRotation(StartSpot->GetActorRotation());
    }
}

void ATeamsGameMode::Logout(AController* Exiting)
{
    ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
    ABlasterPlayerState* ExitingBlasterPlayerState = Exiting->GetPlayerState<ABlasterPlayerState>();
    if (BlasterGameState && ExitingBlasterPlayerState)
    {
        if (BlasterGameState->RedTeam.Contains(ExitingBlasterPlayerState))
        {
            BlasterGameState->RedTeam.Remove(ExitingBlasterPlayerState);
        }
        if (BlasterGameState->BlueTeam.Contains(ExitingBlasterPlayerState))
        {
            BlasterGameState->BlueTeam.Remove(ExitingBlasterPlayerState);
        }
        ExitingBlasterPlayerState->SetTeam(ETeam::ET_NoTeam);
    }

    Super::Logout(Exiting);
}

void ATeamsGameMode::SortPlayerToTeam(ABlasterPlayerState* BlasterPlayerState, ABlasterGameState* BlasterGameState)
{
    if (!BlasterPlayerState || !BlasterGameState) return;
    if (BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
    {
        if (BlasterGameState->BlueTeam.Num() >= BlasterGameState->RedTeam.Num())
        {
            BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
            BlasterGameState->RedTeam.AddUnique(BlasterPlayerState);
        }
        else
        {
            BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
            BlasterGameState->BlueTeam.AddUnique(BlasterPlayerState);
        }
    }
}

float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
    if (Attacker && Victim)
    {
        ABlasterPlayerState* AttackerPState = Attacker->GetPlayerState<ABlasterPlayerState>();
        ABlasterPlayerState* VictimPState = Victim->GetPlayerState<ABlasterPlayerState>();

        if (!AttackerPState || !VictimPState) return BaseDamage;
        if (AttackerPState == VictimPState) return BaseDamage;

        if (AttackerPState->GetTeam() == VictimPState->GetTeam())
        {
            return 0.f;
        }
    }
    return BaseDamage;
}

void ATeamsGameMode::PlayerElimmed(              //
    ABlasterCharacter* ElimmedCharacter,         //
    ABlasterPlayerController* VictimController,  //
    ABlasterPlayerController* AttackerController)
{
    Super::PlayerElimmed(ElimmedCharacter, VictimController, AttackerController);

    // nullptr controllers or suicide
    if (!VictimController || !AttackerController || VictimController == AttackerController) return;

    ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
    ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
    if (BlasterGameState && AttackerPlayerState)
    {
        if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
        {
            BlasterGameState->BlueTeamScores();
        }
        else if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
        {
            BlasterGameState->RedTeamScores();
        }
    }
}

bool ATeamsGameMode::ShouldSpawnAtStartSpot(AController* PlayerController)
{
    if (MatchState == MatchState::EnteringMap || MatchState == MatchState::WaitingToStart)
    {
        return Super::ShouldSpawnAtStartSpot(PlayerController);
    }
    return false;
}

AActor* ATeamsGameMode::GetBestInitializePoint(TArray<AActor*>& PlayerStarts, AController* PlayerController)
{
    if (PlayerStarts.IsEmpty()) return nullptr;
    if (ABlasterPlayerState* BPlayerState = PlayerController->GetPlayerState<ABlasterPlayerState>())
    {
        if (ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)))
        {
            SortPlayerToTeam(BPlayerState, BGameState);
        }

        ETeam PlayerTeam = BPlayerState->GetTeam();
        TArray<AActor*> TeamPlayerStarts;
        FilterPlayerStarts(TeamPlayerStarts, PlayerStarts, PlayerTeam);
        return Super::GetBestInitializePoint(TeamPlayerStarts, PlayerController);
    }
    return nullptr;
}

AActor* ATeamsGameMode::GetBestRespawnPoint(TArray<AActor*>& PlayerStarts, TArray<AActor*>& Players, AController* PlayerController)
{
    if (PlayerStarts.IsEmpty() || Players.IsEmpty()) return nullptr;
    if (ABlasterPlayerState* BPlayerState = PlayerController->GetPlayerState<ABlasterPlayerState>())
    {
        ETeam PlayerTeam = BPlayerState->GetTeam();
        TArray<AActor*> TeamPlayerStarts;
        TArray<AActor*> OppositePlayers;
        FilterPlayerStarts(TeamPlayerStarts, PlayerStarts, PlayerTeam);
        FilterOppositePlayers(OppositePlayers, Players, PlayerTeam);
        return Super::GetBestRespawnPoint(TeamPlayerStarts, OppositePlayers, PlayerController);
    }
    return nullptr;
}

void ATeamsGameMode::FilterPlayerStarts(TArray<AActor*>& OutTeamPlayerStarts, TArray<AActor*>& PlayerStarts, ETeam PlayerTeam)
{
    for (AActor* PlayerStart : PlayerStarts)
    {
        if (ATeamPlayerStart* TeamPlayerStart = Cast<ATeamPlayerStart>(PlayerStart))
        {
            if (TeamPlayerStart->Team == PlayerTeam)
            {
                OutTeamPlayerStarts.Add(TeamPlayerStart);
            }
        }
    }
}

void ATeamsGameMode::FilterOppositePlayers(TArray<AActor*>& OutOppositePlayers, TArray<AActor*>& Players, ETeam PlayerTeam)
{
    for (AActor* Player : Players)
    {
        if (ABlasterCharacter* BCharacter = Cast<ABlasterCharacter>(Player))
        {
            if (BCharacter->GetTeam() != PlayerTeam)
            {
                OutOppositePlayers.Add(BCharacter);
            }
        }
    }
}
