// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "ProjectileWeapon.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
    Super::Fire(HitTarget);

    APawn* InstigatorPawn = Cast<APawn>(GetOwner());
    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
    if (MuzzleFlashSocket && GetWorld() && InstigatorPawn && ProjectileClass)
    {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

        // From muzzle flash socket to hit location from TraceUnderCrosshairs
        FVector ToTarget = HitTarget - SocketTransform.GetLocation();
        FRotator TargetRotation = ToTarget.Rotation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwner();
        SpawnParams.Instigator = InstigatorPawn;

        if (AProjectile* SpawnedProjectile =
                GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams))
        {
            SetProjectileSSR(SpawnedProjectile, InstigatorPawn, SocketTransform.GetLocation());
        }
    }
}

void AProjectileWeapon::SetProjectileSSR(AProjectile* SpawnedProjectile, APawn* InstigatorPawn, FVector_NetQuantize TraceStart)
{
    if (bUseServerSideRewind)
    {
        if (InstigatorPawn->HasAuthority())  // server
        {
            if (InstigatorPawn->IsLocallyControlled())  // server, host - user replicated projectile
            {
                SpawnedProjectile->bUseServerSideRewind = false;
            }
            else  // server, not locally controlled - spawn not - replicated projectile, SSR
            {
                SpawnedProjectile->bUseServerSideRewind = true;
            }
        }
        else  // client, using SSR
        {
            if (InstigatorPawn->IsLocallyControlled())  // client, locally controlled - spawn non - replicated projectile, use SSR
            {
                SpawnedProjectile->bUseServerSideRewind = true;
                SpawnedProjectile->TraceStart = TraceStart;
                SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
            }
            else  // client, not locally controlled - spawn not - replicated projectile, no SSR
            {
                SpawnedProjectile->bUseServerSideRewind = false;
            }
        }
    }
    else  // Weapon not using SSR
    {
        if (InstigatorPawn->HasAuthority())
        {
            SpawnedProjectile->bUseServerSideRewind = false;
        }
    }
}
