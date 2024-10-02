// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HitScanWeapon.h"
#include "Shotgun.generated.h"

UCLASS()
class BLASTER_API AShotgun : public AHitScanWeapon
{
    GENERATED_BODY()

public:
    virtual void Fire(const FVector& HitTarget) override;

private:
    void ApplyMultipleDamage(TMap<ABlasterCharacter*, uint32>& HitMap, AController* InstigatorController);
    void AddToHitMap(FHitResult& FireHit, TMap<ABlasterCharacter*, uint32>& OutHitMap);
    void SpawnImpactFXAndSound(FHitResult& FireHit);

    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    uint32 NumberOfPellets = 10;
};
