// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Components/DecalComponent.h"
#include "ProjectileGrenade.h"

AProjectileGrenade::AProjectileGrenade()
{
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>("Grenade Mesh");
    ProjectileMesh->SetupAttachment(RootComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovementComponent");
    ProjectileMovementComponent->bRotationFollowsVelocity = true;
    ProjectileMovementComponent->SetIsReplicated(true);
    ProjectileMovementComponent->bShouldBounce = true;
}

void AProjectileGrenade::BeginPlay()
{
    AActor::BeginPlay();

    SpawnTrailSystem();
    StartDestoryTimer();

    ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &ThisClass::OnBounce);
}

#if WITH_EDITOR
void AProjectileGrenade::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);

    FName PropertyName = Event.Property ? Event.Property->GetFName() : NAME_None;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileGrenade, InitialSpeed) && ProjectileMovementComponent)
    {
        ProjectileMovementComponent->InitialSpeed = InitialSpeed;
        ProjectileMovementComponent->MaxSpeed = InitialSpeed;
    }
}
#endif

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
    if (BounceSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, BounceSound, GetActorLocation());
    }
}

void AProjectileGrenade::DestroyTimerFinished()
{
    ExplodeDamage();
    Super::SpawnImpactFXAndSound(GetClosestResultToExplosion());
    Super::DestroyTimerFinished();
}

FHitResult AProjectileGrenade::GetClosestResultToExplosion()
{
    TArray<FHitResult> HitResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    bool bHit = GetWorld()->SweepMultiByChannel(HitResults,  //
        GetActorLocation(),                                  //
        GetActorLocation(),                                  //
        FQuat::Identity,                                     //
        ECC_Visibility,                                      //
        FCollisionShape::MakeSphere(TraceDecalRadius),       //
        QueryParams);

    FHitResult ClosestResultToExplosion;
    ClosestResultToExplosion.bBlockingHit = false;
    float ClosestDistanceToActor = 10000.f;

    if (bHit)
    {
        for (const FHitResult& Hit : HitResults)
        {
            if (Hit.GetActor() && Hit.bBlockingHit)
            {
                if (FVector::Dist(GetActorLocation(), Hit.ImpactPoint) < ClosestDistanceToActor)
                {
                    ClosestDistanceToActor = FVector::Dist(GetActorLocation(), Hit.ImpactPoint);
                    ClosestResultToExplosion = Hit;
                }
            }
        }
    }

    return ClosestResultToExplosion;
}
