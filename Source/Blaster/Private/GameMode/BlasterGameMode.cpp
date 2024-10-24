// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "BlasterPlayerState.h"
#include "Engine/World.h"
#include "BlasterCharacter.h"
#include "BlasterGameState.h"
#include "BlasterGameMode.h"

namespace MatchState
{
const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
    bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
    Super::BeginPlay();
    check(GetWorld());

    LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    SetUpMatchState();
}

void ABlasterGameMode::SetUpMatchState()
{
    if (!GetWorld()) return;

    if (MatchState == MatchState::WaitingToStart)
    {
        CountDownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
        if (CountDownTime <= 0.f)
        {
            StartMatch();
        }
    }
    else if (MatchState == MatchState::InProgress)
    {
        CountDownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
        if (CountDownTime <= 0.f)
        {
            SetMatchState(MatchState::Cooldown);
        }
    }
    else if (MatchState == MatchState::Cooldown)
    {
        CountDownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
        if (CountDownTime <= 0.f)
        {
            RestartGame();
        }
    }
}

void ABlasterGameMode::OnMatchStateSet()
{
    Super::OnMatchStateSet();

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*It);
        if (BlasterPlayerController)
        {
            BlasterPlayerController->OnMatchStateSet(MatchState);
        }
    }
}

bool ABlasterGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
    // Test purposes (Start from player start)
    if (GIsEditor)
    {
        return Super::ShouldSpawnAtStartSpot(Player);
    }
    TArray<AActor*> PlayerStarts;
    TArray<AActor*> BlasterCharacters;
    UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
    UGameplayStatics::GetAllActorsOfClass(this, ABlasterCharacter::StaticClass(), BlasterCharacters);
    AActor* BestPlayerStart = GetBestPlayerStart(PlayerStarts, BlasterCharacters);
    if (BestPlayerStart)
    {
        Player->StartSpot = GetBestPlayerStart(PlayerStarts, BlasterCharacters);
        return true;
    }
    return Super::ShouldSpawnAtStartSpot(Player);
}

void ABlasterGameMode::PlayerElimmed(            //
    ABlasterCharacter* ElimmedCharacter,         //
    ABlasterPlayerController* VictimController,  //
    ABlasterPlayerController* AttackerController)
{
    ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
    ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
    ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

    if (!ElimmedCharacter || !AttackerPlayerState || !VictimPlayerState || !BlasterGameState) return;
    if (AttackerPlayerState != VictimPlayerState)
    {
        // killed by other player, not suicide
        AttackerPlayerState->AddToScore(1.f);
        BlasterGameState->UpdateTopScore(AttackerPlayerState);
    }

    VictimPlayerState->AddToDefeats(1);
    VictimPlayerState->AddKilledBy(FName(*AttackerPlayerState->GetPlayerName()));
    ElimmedCharacter->Elim(false);
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
    if (ElimmedCharacter)
    {
        ElimmedCharacter->Reset();
        ElimmedCharacter->Destroy();
    }
    if (ElimmedController)
    {
        TArray<AActor*> PlayerStarts;
        TArray<AActor*> BlasterCharacters;
        UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
        UGameplayStatics::GetAllActorsOfClass(this, ABlasterCharacter::StaticClass(), BlasterCharacters);
        AActor* BestPlayerStart = GetBestPlayerStart(PlayerStarts, BlasterCharacters);
        if (BestPlayerStart)
        {

            RestartPlayerAtPlayerStart(ElimmedController, BestPlayerStart);
        }
    }
}

void ABlasterGameMode::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
    if (!PlayerLeaving) return;
    ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
    if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(PlayerLeaving))
    {
        BlasterGameState->TopScoringPlayers.Remove(PlayerLeaving);
    }
    if (ABlasterCharacter* CharacterLeaving = Cast<ABlasterCharacter>(PlayerLeaving->GetPawn()))
    {
        CharacterLeaving->Elim(true);
    }
}

AActor* ABlasterGameMode::GetBestPlayerStart(TArray<AActor*>& PlayerStarts, TArray<AActor*>& BlasterCharacters)
{
    float MaxDistanceToAllStarts = -1.f;
    AActor* BestPlayerStart = nullptr;
    for (AActor* PlayerStart : PlayerStarts)
    {
        float MinDistanceToOneStart = INFINITY;
        for (AActor* BlasterCharacter : BlasterCharacters)
        {
            float Dist = FVector::Dist(PlayerStart->GetActorLocation(), BlasterCharacter->GetActorLocation());
            MinDistanceToOneStart = FMath::Min(MinDistanceToOneStart, Dist);
        }
        if (MinDistanceToOneStart > MaxDistanceToAllStarts)
        {
            MaxDistanceToAllStarts = MinDistanceToOneStart;
            BestPlayerStart = PlayerStart;
        }
    }
    return BestPlayerStart;
}
