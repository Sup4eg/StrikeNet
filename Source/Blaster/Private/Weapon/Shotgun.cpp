// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
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

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
    if (HitTargets.IsEmpty()) return;
    AWeapon::Fire(FVector());
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;
    AController* InstigatorController = OwnerPawn->GetController();

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (MuzzleFlashSocket && GetWorld())
    {
        const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        const FVector Start = SocketTransform.GetLocation();
        uint32 Hits = 0;

        // Maps hit character to number of times hit
        TMap<ABlasterCharacter*, uint32> HitMap;
        for (const FVector_NetQuantize& HitTarget : HitTargets)
        {
            FHitResult FireHit;
            WeaponTraceHit(Start, HitTarget, FireHit);

            if (FireHit.bBlockingHit)
            {
                AddToHitMap(FireHit, HitMap);
                Super::SpawnImpactFXAndSound(FireHit);
            }
        }
        ApplyMultipleDamage(HitMap, InstigatorController, Start, HitTargets);
    }
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{

    const FVector& TraceStart = GetTraceStart();
    for (uint32 i = 0; i < NumberOfPellets; ++i)
    {
        HitTargets.Add(TraceEndWithScatter(HitTarget, TraceStart));
    }
}

void AShotgun::ApplyMultipleDamage(            //
    TMap<ABlasterCharacter*, uint32>& HitMap,  //
    AController* InstigatorController,         //
    const FVector& Start,                      //
    const TArray<FVector_NetQuantize>& HitTargets)
{
    TArray<ABlasterCharacter*> HitCharacters;
    for (auto HitPair : HitMap)
    {
        if (HitPair.Key)
        {
            if (HasAuthority() && !bUseServerSideRewind && InstigatorController)
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
                                   IsBlasterOwnerControllerValid() &&                       //
                                   BlasterOwnerCharacter->GetLagCompensationComponent() &&  //
                                   !HitCharacters.IsEmpty() &&                              //
                                   !HitTargets.IsEmpty() &&                                 //
                                   BlasterOwnerCharacter->IsLocallyControlled();

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
