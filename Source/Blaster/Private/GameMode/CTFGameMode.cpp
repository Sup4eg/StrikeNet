// Fill out your copyright notice in the Description page of Project Settings.

#include "Flag.h"
#include "FlagZone.h"
#include "Team.h"
#include "BlasterGameState.h"
#include "CTFGameMode.h"

void ACTFGameMode::PlayerElimmed(
    ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
    ABlasterGameMode::PlayerElimmed(ElimmedCharacter, VictimController, AttackerController);
}

void ACTFGameMode::FlagCaptured(AFlag* Flag, AFlagZone* Zone)
{
    bool bValidCapture = Flag->GetTeam() != Zone->Team;
    if (ABlasterGameState* BGameState = Cast<ABlasterGameState>(GameState)) {
        if (Zone->Team == ETeam::ET_BlueTeam) {
            BGameState->BlueTeamScores();
        }
        if (Zone->Team == ETeam::ET_RedTeam) {
            BGameState->RedTeamScores();
        }
    }
}
