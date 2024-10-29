// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

class ABlasterPlayerState;

UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void UpdateTopScore(ABlasterPlayerState* ScoringPlayer);

    void RedTeamScores();
    void BlueTeamScores();

    UFUNCTION()
    void OnRep_RedTeamScore();

    UFUNCTION()
    void OnRep_BlueTeamScore();

    UPROPERTY(Replicated)
    TArray<ABlasterPlayerState*> TopScoringPlayers;

    /** Teams */
    TArray<ABlasterPlayerState*> RedTeam;
    TArray<ABlasterPlayerState*> BlueTeam;

    UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
    float RedTeamScore = 0.f;

    UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
    float BlueTeamScore = 0.f;

private:
    float TopScore = 0;
};
