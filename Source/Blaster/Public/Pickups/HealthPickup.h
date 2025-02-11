// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "HealthPickup.generated.h"

UCLASS()
class BLASTER_API AHealthPickup : public APickup
{
    GENERATED_BODY()

public:
    AHealthPickup();

protected:
    virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
        AActor* OtherActor,                                                 //
        UPrimitiveComponent* OtherComp,                                     //
        int32 OtherBodyIndex,                                               //
        bool bFromSweep,                                                    //
        const FHitResult& SweepResult) override;

private:
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0.f))
    float HealAmount = 100.f;

    UPROPERTY(EditAnywhere, meta = (ClampMin = 1.f))
    float HealingTime = 5.f;
};
