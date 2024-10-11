// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "DeathVolume.generated.h"

UCLASS()
class BLASTER_API ADeathVolume : public ATriggerBox
{
    GENERATED_BODY()

public:
    ADeathVolume();

    virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
};
