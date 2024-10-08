// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "InvisibilityPickup.generated.h"

UCLASS()
class BLASTER_API AInvisibilityPickup : public APickup
{
    GENERATED_BODY()

public:
    AInvisibilityPickup();

protected:
    virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
        AActor* OtherActor,                                                 //
        UPrimitiveComponent* OtherComp,                                     //
        int32 OtherBodyIndex,                                               //
        bool bFromSweep,                                                    //
        const FHitResult& SweepResult) override;

private:
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0.f, ClampMax = 1.f))
    float Opacity = 0.05f;

    UPROPERTY(EditAnywhere)
    float InvisibilityBuffTime = 30.f;
};
