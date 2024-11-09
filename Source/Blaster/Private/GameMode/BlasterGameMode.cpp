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
            BlasterPlayerController->OnMatchStateSet(MatchState, bTeamsMatch);
        }
    }
}

bool ABlasterGameMode::ShouldSpawnAtStartSpot(AController* PlayerController)
{
    // Test purposes (Start from player start)
    // if (GIsEditor)
    // {
    //     return Super::ShouldSpawnAtStartSpot(PlayerController);
    // }
    TArray<AActor*> PlayerStarts;
    UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
    AActor* BestPlayerStart = GetBestInitializePoint(PlayerStarts, PlayerController);
    if (BestPlayerStart)
    {
        PlayerController->StartSpot = BestPlayerStart;
        return true;
    }
    return Super::ShouldSpawnAtStartSpot(PlayerController);
}

void ABlasterGameMode::PlayerElimmed(            //
    ABlasterCharacter* ElimmedCharacter,         //
    ABlasterPlayerController* VictimController,  //
    ABlasterPlayerController* AttackerController)
{
    ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
    ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
    ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

    if (!ElimmedCharacter || !AttackerPlayerState || !VictimPlayerState || !BlasterGameState || !GetWorld()) return;
    if (AttackerPlayerState != VictimPlayerState)
    {
        TArray<ABlasterPlayerState*> PlayersCurrentlyInTheLead;
        for (auto& LeadPlayer : BlasterGameState->TopScoringPlayers)
        {
            PlayersCurrentlyInTheLead.Add(LeadPlayer);
        }

        // killed by other player, not suicide
        AttackerPlayerState->AddToScore(1.f);
        BlasterGameState->UpdateTopScore(AttackerPlayerState);
        if (BlasterGameState->TopScoringPlayers.Contains(AttackerPlayerState))
        {
            if (ABlasterCharacter* Leader = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn()))
            {
                Leader->MulticastGainedTheLead();
            }
        }

        for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); ++i)
        {
            if (!BlasterGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
            {
                if (ABlasterCharacter* Loser = Cast<ABlasterCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn()))
                {
                    Loser->MulticastLostTheLead();
                }
            }
        }
    }

    VictimPlayerState->AddToDefeats(1);
    VictimPlayerState->AddKilledBy(FName(*AttackerPlayerState->GetPlayerName()));
    ElimmedCharacter->Elim(false);

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*It))
        {
            BlasterPlayerController->BroadcastElim(AttackerPlayerState, VictimPlayerState);
        }
    }
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
        TArray<AActor*> Players;
        UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
        UGameplayStatics::GetAllActorsOfClass(this, ABlasterCharacter::StaticClass(), Players);
        AActor* BestPlayerStart = GetBestRespawnPoint(PlayerStarts, Players, ElimmedController);
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

float ABlasterGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
    return BaseDamage;
}

AActor* ABlasterGameMode::GetBestInitializePoint(TArray<AActor*>& PlayerStarts, AController* PlayerController)
{
    if (PlayerStarts.IsEmpty()) return nullptr;
    const APawn* PawnToFit = ABlasterCharacter::StaticClass()->GetDefaultObject<APawn>();
    AActor* BestPlayerStart = nullptr;
    ShuffleActorArray(PlayerStarts);
    for (AActor* PlayerStart : PlayerStarts)
    {
        if (GetWorld() &&
            GetWorld()->EncroachingBlockingGeometry(PawnToFit, PlayerStart->GetActorLocation(), PlayerStart->GetActorRotation()))
        {
            continue;
        }
        BestPlayerStart = PlayerStart;
        break;
    }
    return BestPlayerStart;
}

AActor* ABlasterGameMode::GetBestRespawnPoint(TArray<AActor*>& PlayerStarts, TArray<AActor*>& Players, AController* PlayerContoller)
{
    if (PlayerStarts.IsEmpty() || Players.IsEmpty()) return nullptr;

    const APawn* PawnToFit = ABlasterCharacter::StaticClass()->GetDefaultObject<APawn>();

    float MaxDistanceToAllStarts = -1.f;
    AActor* BestPlayerStart = nullptr;
    for (AActor* PlayerStart : PlayerStarts)
    {
        if (GetWorld() &&
            GetWorld()->EncroachingBlockingGeometry(PawnToFit, PlayerStart->GetActorLocation(), PlayerStart->GetActorRotation()))
        {
            continue;
        }

        float MinDistanceToOneStart = INFINITY;
        for (AActor* BlasterCharacter : Players)
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
    // no room for player start
    if (!BestPlayerStart)
    {
        BestPlayerStart = PlayerStarts[FMath::RandRange(0, PlayerStarts.Num() - 1)];
    }
    return BestPlayerStart;
}

void ABlasterGameMode::ShuffleActorArray(TArray<AActor*>& ActorArr)
{
    int32 LastIndex = ActorArr.Num() - 1;
    for (int32 i = LastIndex; i > 0; --i)
    {
        int32 RandomIndex = FMath::RandRange(0, i);
        ActorArr.Swap(i, RandomIndex);
    }
}
