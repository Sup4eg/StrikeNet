// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "BlasterPlayerState.h"
#include "BlasterCharacter.h"
#include "BlasterGameMode.h"

void ABlasterGameMode::PlayerEliminated(         //
    ABlasterCharacter* ElimmedCharacter,         //
    ABlasterPlayerController* VictimController,  //
    ABlasterPlayerController* AttackerController)
{
    ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
    ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

    if (!ElimmedCharacter || !AttackerPlayerState || !VictimPlayerState || AttackerPlayerState == VictimPlayerState) return;
    AttackerPlayerState->AddToScore(1.f);
    VictimPlayerState->AddToDefeats(1);
    VictimPlayerState->AddKilledBy(FName(*AttackerPlayerState->GetPlayerName()));
    ElimmedCharacter->Elim();
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
        if (!PlayerStarts.IsEmpty())
        {

            RestartPlayerAtPlayerStart(ElimmedController, GetBestPlayerStart(PlayerStarts, BlasterCharacters));
        }
    }
}

AActor* ABlasterGameMode::GetBestPlayerStart(TArray<AActor*>& PlayerStarts, TArray<AActor*>& BlasterCharacters)
{
    float MaxDistanceToAllStarts = -1.f;
    AActor* BestPlayerStart = BlasterCharacters[0];
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
