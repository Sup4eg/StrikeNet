// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterCharacter.h"
#include "BuffComponent.h"
#include "HealthPickup.h"

AHealthPickup::AHealthPickup()
{
    bReplicates = true;
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
    AActor* OtherActor,                                                        //
    UPrimitiveComponent* OtherComp,                                            //
    int32 OtherBodyIndex,                                                      //
    bool bFromSweep,                                                           //
    const FHitResult& SweepResult)
{
    Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    if (OtherActor && OtherActor->ActorHasTag("BlasterCharacter") && HasAuthority())
    {
        if (IsBlasterCharacterValid(OtherActor))
        {
            if (UBuffComponent* BuffComp = BlasterCharacter->GetBuffComponent())
            {
                BuffComp->Heal(HealAmount, HealingTime);
                Destroy();
            }
        }
    }
}
