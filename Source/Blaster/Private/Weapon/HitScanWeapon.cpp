// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/World.h"
#include "BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "DrawDebugHelpers.h"
#include "WeaponTypes.h"
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
            if (HasAuthority() && InstigatorController && FireHit.GetActor()->ActorHasTag("BlasterCharacter"))
            {
                if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
                {
                    UGameplayStatics::ApplyDamage(BlasterCharacter, Damage, InstigatorController, this, UDamageType::StaticClass());
                }
            }
            if (ImpactParticles)
            {
                UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
            }
            if (HitSound)
            {
                UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint);
            }
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
    FVector TraceEnd = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;
    GetWorld()->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility);
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

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
    FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
    FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
    FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
    FVector EndLoc = SphereCenter + RandVec;
    FVector ToEndLoc = EndLoc - TraceStart;
    /*
    DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
    DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);
    DrawDebugLine(GetWorld(),                                    //
        TraceStart,                                              //
        TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size(),  //
        FColor::Cyan,                                            //
        true);
    */
    return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}
