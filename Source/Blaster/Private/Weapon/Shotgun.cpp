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
#include "Shotgun.h"

void AShotgun::Fire(const FVector& HitTarget)
{
    AWeapon::Fire(HitTarget);
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;
    AController* InstigatorController = OwnerPawn->GetController();

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (MuzzleFlashSocket && GetWorld())
    {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        FVector Start = SocketTransform.GetLocation();
        uint32 Hits = 0;

        TMap<ABlasterCharacter*, uint32> HitMap;
        for (uint32 i = 0; i < NumberOfPellets; ++i)
        {
            FHitResult FireHit;
            WeaponTraceHit(Start, HitTarget, FireHit);

            if (FireHit.bBlockingHit)
            {
                AddToHitMap(FireHit, HitMap);
                Super::SpawnImpactFXAndSound(FireHit);
            }
        }
        ApplyMultipleDamage(HitMap, InstigatorController);
    }
}

void AShotgun::ApplyMultipleDamage(TMap<ABlasterCharacter*, uint32>& HitMap, AController* InstigatorController)
{
    if (InstigatorController && HasAuthority())
    {
        for (auto HitPair : HitMap)
        {
            if (HitPair.Key)
            {
                UGameplayStatics::ApplyDamage(HitPair.Key,  //
                    Damage * HitPair.Value,                 //
                    InstigatorController,                   //
                    this,                                   //
                    UDamageType::StaticClass());
            }
        }
    }
}

void AShotgun::AddToHitMap(FHitResult& FireHit, TMap<ABlasterCharacter*, uint32>& OutHitMap)
{
    if (FireHit.GetActor()->ActorHasTag("BlasterCharacter"))
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
