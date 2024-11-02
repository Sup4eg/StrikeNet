// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
class ABlasterPlayerState;

namespace MatchState
{
extern BLASTER_API const FName Cooldown;  // Match duration has been reached. Display the winner and begin cooldown timer.
}

UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    ABlasterGameMode();

    virtual void Tick(float DeltaTime) override;

    virtual void PlayerElimmed(                      //
        ABlasterCharacter* ElimmedCharacter,         //
        ABlasterPlayerController* VictimController,  //
        ABlasterPlayerController* AttackerController);

    virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);

    void PlayerLeftGame(ABlasterPlayerState* PlayerLeaving);

    virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);

    UPROPERTY(EditDefaultsOnly)
    float WarmupTime = 10.f;

    UPROPERTY(EditDefaultsOnly)
    float MatchTime = 120.f;

    UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
    float MatchTimeLeftAlert = 30;

    UPROPERTY(EditDefaultsOnly)
    float CooldownTime = 10.f;

    float LevelStartingTime = 0.f;

protected:
    virtual void BeginPlay() override;
    virtual void OnMatchStateSet() override;
    virtual bool ShouldSpawnAtStartSpot(AController* PlayerController) override;
    virtual AActor* GetBestInitializePoint(TArray<AActor*>& PlayerStarts, AController* PlayerController);
    virtual AActor* GetBestRespawnPoint(TArray<AActor*>& PlayerStarts, TArray<AActor*>& Players, AController* PlayerController);

    // TODO : use ENUM here??
    bool bTeamsMatch = false;

private:
    void SetUpMatchState();

    void ShuffleActorArray(TArray<AActor*>& ActorArr);

    float CountDownTime = 0.f;

public:
    FORCEINLINE float GetCountdownTime() const { return CountDownTime; };
};
