// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterCharacter.h"
#include "BuffComp.h"
#include "SpeedPickup.h"

ASpeedPickup::ASpeedPickup()
{
    bReplicates = true;
}

void ASpeedPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
    AActor* OtherActor,                                                       //
    UPrimitiveComponent* OtherComp,                                           //
    int32 OtherBodyIndex,                                                     //
    bool bFromSweep,                                                          //
    const FHitResult& SweepResult)
{
    Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    if (OtherActor && OtherActor->ActorHasTag("BlasterCharacter") && HasAuthority())
    {
        if (IsBlasterCharacterValid(OtherActor))
        {
            if (UBuffComp* BuffComp = BlasterCharacter->GetBuffComponent())
            {
                BuffComp->BuffSpeed(BuffSpeedScaleFactor, SpeedBuffTime);
                Destroy();
            }
        }
    }
}