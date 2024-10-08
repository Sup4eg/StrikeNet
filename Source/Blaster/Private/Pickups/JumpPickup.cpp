// Fill out your copyright notice in the Description page of Project Settings.

#include "BuffComp.h"
#include "BlasterCharacter.h"
#include "JumpPickup.h"

AJumpPickup::AJumpPickup()
{
    bReplicates = true;
}

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
    AActor* OtherActor,                                                      //
    UPrimitiveComponent* OtherComp,                                          //
    int32 OtherBodyIndex,                                                    //
    bool bFromSweep,                                                         //
    const FHitResult& SweepResult)
{
    Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    if (OtherActor && OtherActor->ActorHasTag("BlasterCharacter") && HasAuthority())
    {
        if (IsBlasterCharacterValid(OtherActor))
        {
            if (UBuffComp* BuffComp = BlasterCharacter->GetBuffComponent())
            {
                BuffComp->BuffJump(BuffJumpScaleFactor, JumpBuffTime);
                Destroy();
            }
        }
    }
}
