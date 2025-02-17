// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/World.h"
#include "BlasterCharacter.h"
#include "BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "DrawDebugHelpers.h"
#include "Components/DecalComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CarryItemTypes.h"
#include "LagCompensationComponent.h"
#include "HitScanWeapon.h"

void AHitScanWeapon::Fire(const FVector_NetQuantize100& HitTarget, const FVector_NetQuantize100& SocketLocation)
{
    Super::Fire(HitTarget, SocketLocation);

    //DrawDebugLine(GetWorld(), SocketLocation, HitTarget, FColor::Red, true, 5.f, 0, 5.f);

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;
    AController* InstigatorController = OwnerPawn->GetController();
    if (GetWorld())
    {
        FHitResult FireHit;
        WeaponTraceHit(SocketLocation, HitTarget, FireHit);

        if (FireHit.bBlockingHit)
        {
            if (InstigatorController && FireHit.GetActor()->ActorHasTag("BlasterCharacter"))
            {
                if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
                {
                    bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
                    // Apply Damage on server
                    if (HasAuthority() && bCauseAuthDamage && FireHit.PhysMaterial.IsValid())
                    {

                        UPhysicalMaterial* PhysMat = FireHit.PhysMaterial.Get();
                        if (HitCharacter->DamageModifiers.Contains(PhysMat))
                        {
                            UGameplayStatics::ApplyDamage(HitCharacter,           //
                                Damage * HitCharacter->DamageModifiers[PhysMat],  //                                      //
                                InstigatorController,                             //
                                this,                                             //
                                UDamageType::StaticClass());
                        }
                    }
                    // Apply Damage on client, use SSR
                    else if (!HasAuthority() &&                                     //
                             bUseServerSideRewind &&                                //
                             BlasterOwnerCharacter->IsLocallyControlled() &&        //
                             IsBlasterOwnerControllerValid() &&                     //
                             BlasterOwnerCharacter->GetLagCompensationComponent())  //
                    {

                        float HitTime = BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime;

                        // Hacked, check validation
                        // Damage = 10000;

                        BlasterOwnerCharacter->GetLagCompensationComponent()->ServerScoreRequest(  //
                            HitCharacter,                                                          //
                            SocketLocation,                                                        //
                            HitTarget,                                                             //
                            HitTime,                                                               //
                            Damage,                                                                //
                            this);
                    }
                }
            }
            SpawnImpactFXAndSound(FireHit);
        }

        if (MuzzleFlash)
        {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, GetLocalWeaponSocketTransform());
        }
        if (FireSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
        }
    }
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector_NetQuantize100& HitTarget, FHitResult& OutHit)
{
    if (!GetWorld()) return;

    FCollisionQueryParams Params;
    Params.bReturnPhysicalMaterial = true;

    FVector TraceEnd = TraceStart + (HitTarget - TraceStart) * 1.25f;
    GetWorld()->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility, Params);
    FVector BeamEnd = TraceEnd;
    if (OutHit.bBlockingHit)
    {
        BeamEnd = OutHit.ImpactPoint;
    }

    if (BeamParticles)
    {
        if (UParticleSystemComponent* Beam =
                UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, GetLocalWeaponSocketTransform(), true))
        {
            Beam->SetVectorParameter("Target", BeamEnd);
        }
    }
}

void AHitScanWeapon::SpawnImpactFXAndSound(FHitResult& FireHit)
{

    FImpactData ImpactData = GetImpactData(FireHit);

    SpawnImpactParticles(FireHit, ImpactData);
    SpawnImpactSound(FireHit, ImpactData);
    SpawnImpactDecal(FireHit, ImpactData);
}

void AHitScanWeapon::SpawnImpactParticles(FHitResult& FireHit, FImpactData& ImpactData)
{
    if (ImpactData.ImpactParticles && GetWorld())
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(), ImpactData.ImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
    }
}

void AHitScanWeapon::SpawnImpactSound(FHitResult& FireHit, FImpactData& ImpactData)
{
    if (ImpactData.ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ImpactData.ImpactSound, FireHit.ImpactPoint);
    }
}

void AHitScanWeapon::SpawnImpactDecal(FHitResult& FireHit, FImpactData& ImpactData)
{
    if (ImpactData.DecalData.Material && GetWorld())
    {
        UDecalComponent* DecalComponent = UGameplayStatics::SpawnDecalAtLocation(GetWorld(),  //
            ImpactData.DecalData.Material,                                                    //
            ImpactData.DecalData.Size,                                                        //
            FireHit.ImpactPoint,                                                              //
            FireHit.ImpactNormal.Rotation());

        if (DecalComponent)
        {
            DecalComponent->SetFadeScreenSize(0.f);
            DecalComponent->SetFadeOut(ImpactData.DecalData.LifeTime, ImpactData.DecalData.FadeOutTime);
        }
    }
}

FImpactData AHitScanWeapon::GetImpactData(FHitResult& FireHit)
{
    FImpactData ImpactData = DefaultImpactData;

    if (FireHit.PhysMaterial.IsValid())
    {
        UPhysicalMaterial* PhysMat = FireHit.PhysMaterial.Get();
        if (ImpactDataMap.Contains(PhysMat))
        {
            ImpactData = ImpactDataMap[PhysMat];
        }
    }

    return ImpactData;
}

FTransform AHitScanWeapon::GetLocalWeaponSocketTransform()
{
    if (GetItemMesh())
    {
        const USkeletalMeshSocket* MuzzleFlashSocket = GetItemMesh()->GetSocketByName(FName("MuzzleFlash"));
        if (MuzzleFlashSocket)
        {
            FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetItemMesh());
            return SocketTransform;
        }
    }
    return FTransform();
}
