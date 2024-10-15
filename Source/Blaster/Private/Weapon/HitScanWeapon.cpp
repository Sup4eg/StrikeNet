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
#include "WeaponTypes.h"
#include "LagCompensationComponent.h"
#include "HitScanWeapon.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
    Super::Fire(HitTarget);

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;
    AController* InstigatorController = OwnerPawn->GetController();

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (MuzzleFlashSocket && GetWorld())
    {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        FVector Start = SocketTransform.GetLocation();

        FHitResult FireHit;
        WeaponTraceHit(Start, HitTarget, FireHit);

        if (FireHit.bBlockingHit)
        {
            if (InstigatorController && FireHit.GetActor()->ActorHasTag("BlasterCharacter"))
            {
                if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
                {
                    // Apply Damage on server
                    if (HasAuthority() && !bUseServerSideRewind)
                    {
                        UGameplayStatics::ApplyDamage(BlasterCharacter,  //
                            Damage,                                      //
                            InstigatorController,                        //
                            this,                                        //
                            UDamageType::StaticClass());
                    }
                    // Apply Damage on client, use SSR
                    else if (!HasAuthority() &&                                       //
                             bUseServerSideRewind &&                                  //
                             IsBlasterOwnerControllerValid() &&                       //
                             BlasterOwnerCharacter->GetLagCompensationComponent() &&  //
                             BlasterOwnerCharacter->IsLocallyControlled())
                    {
                        float HitTime = BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime;

                        BlasterOwnerCharacter->GetLagCompensationComponent()->ServerScoreRequest(  //
                            BlasterCharacter,                                                      //
                            Start,                                                                 //
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
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
        }
        if (FireSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
        }
    }
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
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
                UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, TraceStart, FRotator::ZeroRotator, true))
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
