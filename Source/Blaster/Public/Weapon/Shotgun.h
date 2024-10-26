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
    virtual void FireShotgun(const TArray<FVector_NetQuantize100>& HitTargets, const FVector_NetQuantize100& SocketLocation);
    void ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize100>& HitTargets);

protected:
    virtual void SpawnImpactSound(FHitResult& FireHit, FImpactData& ImpactData) override;

private:
    void ApplyMultipleDamage(                          //
        TMap<ABlasterCharacter*, float>& HitMap,      //
        APawn* OwnerPawn,                              //
        AController* InstigatorController,             //
        const FVector& Start,                          //
        const TArray<FVector_NetQuantize100>& HitTargets  //
    );

    void AddToHitMap(FHitResult& FireHit, TMap<ABlasterCharacter*, float>& OutHitMap);

    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    uint32 NumberOfPellets = 10;
};
