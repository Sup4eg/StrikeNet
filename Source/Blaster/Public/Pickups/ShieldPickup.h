// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "ShieldPickup.generated.h"

UCLASS()
class BLASTER_API AShieldPickup : public APickup
{
    GENERATED_BODY()

public:
    AShieldPickup();

protected:
    virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
        AActor* OtherActor,                                                 //
        UPrimitiveComponent* OtherComp,                                     //
        int32 OtherBodyIndex,                                               //
        bool bFromSweep,                                                    //
        const FHitResult& SweepResult) override;

private:
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0.f))
    float ShieldReplenishAmount = 100.f;

    UPROPERTY(EditAnywhere, meta = (ClampMin = 1.f))
    float ShieldReplenishTime = 5.f;
};
