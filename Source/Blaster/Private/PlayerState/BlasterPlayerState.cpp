// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterCharacter.h"
#include "BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "BlasterUtils.h"
#include "BlasterPlayerState.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABlasterPlayerState, Defeats);
    DOREPLIFETIME(ABlasterPlayerState, KilledBy);
    DOREPLIFETIME(ABlasterPlayerState, Team);
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
    SetScore(GetScore() + ScoreAmount);
    HandleScore();
}

void ABlasterPlayerState::AddToDefeats(float DefeatsAmount)
{
    SetDefeats(GetDefeats() + DefeatsAmount);
    HandleDefeats();
}

void ABlasterPlayerState::AddKilledBy(const FName& NewKilledBy)
{
    KilledBy = NewKilledBy;
    HandleKilledBy();
}

void ABlasterPlayerState::OnRep_Score()
{
    Super::OnRep_Score();
    HandleScore();
}

void ABlasterPlayerState::OnRep_Defeats()
{
    HandleDefeats();
}

void ABlasterPlayerState::OnRep_KilledBy()
{
    HandleKilledBy();
}

void ABlasterPlayerState::HandleScore()
{
    if (IsControllerValid())
    {
        Controller->SetHUDScore(GetScore());
    }
}

void ABlasterPlayerState::HandleDefeats()
{
    if (IsControllerValid())
    {
        Controller->SetHUDDefeats(GetDefeats());
    }
}

void ABlasterPlayerState::HandleKilledBy()
{
    if (IsControllerValid())
    {
        Controller->ShowHUDElimmed(GetKilledBy());
    }
}

bool ABlasterPlayerState::IsControllerValid()
{
    return BlasterUtils::CastOrUseExistsActors<ABlasterCharacter, ABlasterPlayerController>(Character, Controller, GetPawn());
}

void ABlasterPlayerState::SetTeam(ETeam NewTeam)
{
    Team = NewTeam;

    if (ABlasterCharacter* BCharacter = Cast<ABlasterCharacter>(GetPawn()))
    {
        BCharacter->SetTeamColor(Team);
    }
}

void ABlasterPlayerState::OnRep_Team()
{
    if (ABlasterCharacter* BCharacter = Cast<ABlasterCharacter>(GetPawn()))
    {
        BCharacter->SetTeamColor(Team);
    }
}
