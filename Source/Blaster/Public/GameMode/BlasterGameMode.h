// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;

UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    virtual void PlayerEliminated(                   //
        ABlasterCharacter* ElimmedCharacter,         //
        ABlasterPlayerController* VictimController,  //
        ABlasterPlayerController* AttackerController);

    virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);

private:
    AActor* GetBestPlayerStart(TArray<AActor*>& PlayerStarts, TArray<AActor*>& BlasterCharacters);
};
