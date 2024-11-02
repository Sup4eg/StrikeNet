// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Team.h"
#include "FlagZone.generated.h"

class USphereComponent;

UCLASS()
class BLASTER_API AFlagZone : public AActor
{
    GENERATED_BODY()

public:
    AFlagZone();

	UPROPERTY(EditAnywhere)
	ETeam Team;
protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    virtual void OnsphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
        AActor* OtherActor,                                                 //
        UPrimitiveComponent* OtherComp,                                     //
        int32 OtherBodyIndex,                                               //
        bool bFromSweep,                                                    //
        const FHitResult& SweepResult);

private:
    UPROPERTY(EditAnywhere)
    USphereComponent* ZoneSphere;
};
