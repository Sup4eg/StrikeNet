// Fill out your copyright notice in the Description page of Project Settings.

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "BlasterCharacter.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"
#include "GameFramework/DamageType.h"
#include "Components/DecalComponent.h"
#include "BlasterPlayerController.h"
#include "LagCompensationComponent.h"
#include "Shotgun.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize100>& HitTargets, const FVector_NetQuantize100& SocketLocation)
{
    if (HitTargets.IsEmpty()) return;
    AWeapon::Fire(FVector(), FVector());
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;
    AController* InstigatorController = OwnerPawn->GetController();

    if (GetWorld())
    {
        uint32 Hits = 0;

        // Maps hit character to number of times hit
        TMap<ABlasterCharacter*, uint32> HitMap;
        for (const FVector_NetQuantize100& HitTarget : HitTargets)
        {
            FHitResult FireHit;
            WeaponTraceHit(SocketLocation, HitTarget, FireHit);

            if (FireHit.bBlockingHit)
            {
                AddToHitMap(FireHit, HitMap);
                Super::SpawnImpactFXAndSound(FireHit);
            }
        }
        ApplyMultipleDamage(HitMap, OwnerPawn, InstigatorController, SocketLocation, HitTargets);
    }
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize100>& HitTargets)
{

    const FVector& TraceStart = GetTraceStart();
    for (uint32 i = 0; i < NumberOfPellets; ++i)
    {
        HitTargets.Add(TraceEndWithScatter(HitTarget, TraceStart));
    }
}

void AShotgun::ApplyMultipleDamage(            //
    TMap<ABlasterCharacter*, uint32>& HitMap,  //
    APawn* OwnerPawn,                          //
    AController* InstigatorController,         //
    const FVector& Start,                      //
    const TArray<FVector_NetQuantize100>& HitTargets)
{
    TArray<ABlasterCharacter*> HitCharacters;
    for (auto HitPair : HitMap)
    {
        if (HitPair.Key && OwnerPawn)
        {
            bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();

            if (HasAuthority() && bCauseAuthDamage && InstigatorController)
            {
                UGameplayStatics::ApplyDamage(HitPair.Key,  //
                    Damage * HitPair.Value,                 //
                    InstigatorController,                   //
                    this,                                   //
                    UDamageType::StaticClass());
            }
            HitCharacters.Add(HitPair.Key);
        }
    }

    bool bServerSideRewindDamage = !HasAuthority() &&                                       //
                                   bUseServerSideRewind &&                                  //
                                   BlasterOwnerCharacter->IsLocallyControlled() &&          //
                                   IsBlasterOwnerControllerValid() &&                       //
                                   BlasterOwnerCharacter->GetLagCompensationComponent() &&  //
                                   !HitCharacters.IsEmpty() &&                              //
                                   !HitTargets.IsEmpty();                                   //

    if (bServerSideRewindDamage)
    {
        float HitTime = BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime;

        BlasterOwnerCharacter->GetLagCompensationComponent()->ShotgunServerScoreRequest(  //
            HitCharacters,                                                                //
            Start,                                                                        //
            HitTargets,                                                                   //
            HitTime,                                                                      //
            Damage,                                                                       //
            this);
    }
}

void AShotgun::AddToHitMap(FHitResult& FireHit, TMap<ABlasterCharacter*, uint32>& OutHitMap)
{
    if (FireHit.GetActor() && FireHit.GetActor()->ActorHasTag("BlasterCharacter"))
    {
        if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
        {
            if (OutHitMap.Contains(BlasterCharacter))
            {
                ++OutHitMap[BlasterCharacter];
            }
            else
            {
                OutHitMap.Emplace(BlasterCharacter, 1);
            }
        }
    }
}

void AShotgun::SpawnImpactSound(FHitResult& FireHit, FImpactData& ImpactData)
{
    if (ImpactData.ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ImpactData.ImpactSound, FireHit.ImpactPoint, .5f, FMath::FRandRange(-.5f, .5f));
    }
}
