// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterGameState.h"
#include "BlasterPlayerState.h"
#include "BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TeamsGameMode.h"

ATeamsGameMode::ATeamsGameMode()
{
    bTeamsMatch = true;
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (!NewPlayer) return;
    ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
    ABlasterPlayerState* NewBlasterPlayerState = NewPlayer->GetPlayerState<ABlasterPlayerState>();
    SortPlayerToTeam(NewBlasterPlayerState, BlasterGameState);
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
    }

    Super::Logout(Exiting);
}

void ATeamsGameMode::HandleMatchHasStarted()
{
    Super::HandleMatchHasStarted();

    if (ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)))
    {
        for (auto& PlayerState : BlasterGameState->PlayerArray)
        {
            if (ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(PlayerState))
            {
                SortPlayerToTeam(BlasterPlayerState, BlasterGameState);
            }
        }
    }
}

void ATeamsGameMode::SortPlayerToTeam(ABlasterPlayerState* BlasterPlayerState, ABlasterGameState* BlasterGameState)
{
    if (!BlasterPlayerState || !BlasterGameState) return;
    if (BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
    {
        if (BlasterGameState->BlueTeam.Num() >= BlasterGameState->RedTeam.Num())
        {
            BlasterGameState->RedTeam.AddUnique(BlasterPlayerState);
            BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
        }
        else
        {
            BlasterGameState->BlueTeam.AddUnique(BlasterPlayerState);
            BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
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

    //nullptr controllers or suicide
    if (!VictimController || !AttackerController || VictimController == AttackerController) return;

    ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
    ABlasterPlayerState* AttackerPlayerState =
        AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
    if (BlasterGameState && AttackerPlayerState)
    {
        if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam) {
            BlasterGameState->BlueTeamScores();
        } else if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam) {
            BlasterGameState->RedTeamScores();
        }
    }
}
