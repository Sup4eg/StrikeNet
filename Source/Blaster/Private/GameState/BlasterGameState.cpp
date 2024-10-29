// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterPlayerState.h"
#include "Engine/World.h"
#include "BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "BlasterGameState.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABlasterGameState, TopScoringPlayers);
    DOREPLIFETIME(ABlasterGameState, RedTeamScore);
    DOREPLIFETIME(ABlasterGameState, BlueTeamScore);
}

void ABlasterGameState::UpdateTopScore(ABlasterPlayerState* ScoringPlayer)
{
    if (TopScoringPlayers.IsEmpty())
    {
        TopScoringPlayers.Add(ScoringPlayer);
        TopScore = ScoringPlayer->GetScore();
    }
    else if (ScoringPlayer->GetScore() == TopScore)
    {
        TopScoringPlayers.AddUnique(ScoringPlayer);
    }
    else if (ScoringPlayer->GetScore() > TopScore)
    {
        TopScoringPlayers.Empty();
        TopScoringPlayers.Add(ScoringPlayer);
        TopScore = ScoringPlayer->GetScore();
    }
}

void ABlasterGameState::RedTeamScores()
{
    if (!GetWorld()) return;
    ++RedTeamScore;
    if (ABlasterPlayerController* BPlayer = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        BPlayer->SetHUDRedTeamScore(RedTeamScore);
    }
}

void ABlasterGameState::BlueTeamScores()
{
    if (!GetWorld()) return;
    ++BlueTeamScore;
    if (ABlasterPlayerController* BPlayer = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        BPlayer->SetHUDBlueTeamScore(BlueTeamScore);
    }
}

void ABlasterGameState::OnRep_RedTeamScore()
{
    if (!GetWorld()) return;
    if (ABlasterPlayerController* BPlayer = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        BPlayer->SetHUDRedTeamScore(RedTeamScore);
    }
}

void ABlasterGameState::OnRep_BlueTeamScore()
{
    if (!GetWorld()) return;
    if (ABlasterPlayerController* BPlayer = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        BPlayer->SetHUDBlueTeamScore(BlueTeamScore);
    }
}
