// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;

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

    UPROPERTY(EditDefaultsOnly)
    float WarmupTime = 10.f;

    UPROPERTY(EditDefaultsOnly)
    float MatchTime = 120.f;

    UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0", ClampMax = "1.0"))
    float MatchTimeLeftAlertPercentage = 0.25f;

    UPROPERTY(EditDefaultsOnly)
    float CooldownTime = 10.f;

    float LevelStartingTime = 0.f;

protected:
    virtual void BeginPlay() override;
    virtual void OnMatchStateSet() override;
    virtual bool ShouldSpawnAtStartSpot(AController* Player) override;

private:
    float CountDownTime = 0.f;

    AActor* GetBestPlayerStart(TArray<AActor*>& PlayerStarts, TArray<AActor*>& BlasterCharacters);

    void SetUpMatchState();

public:
    FORCEINLINE float GetCountdownTime() const { return CountDownTime; };
};
