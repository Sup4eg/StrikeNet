// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Blaster.h"
#include "Projectile.h"

AProjectile::AProjectile()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>("CollisionBox");
    SetRootComponent(CollisionBox);

    CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
    CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
    CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
    CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
}

void AProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (Tracer)
    {
        TracerComponent = UGameplayStatics::SpawnEmitterAttached(
            Tracer, CollisionBox, FName(), GetActorLocation(), GetActorRotation(), EAttachLocation::KeepWorldPosition);
    }

    if (HasAuthority())
    {
        CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
        CollisionBox->IgnoreActorWhenMoving(Owner, true);

        SetLifeSpan(LifeSpan);
    }
}

void AProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AProjectile::OnHit(
    UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    MulticastHit(OtherActor);
    Destroy();
}

void AProjectile::MulticastHit_Implementation(AActor* OtherActor)
{
    PlayFXAndSound(OtherActor);
}

void AProjectile::SpawnTrailSystem()
{
    if (TrailSystem)
    {
        TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(  //
            TrailSystem,                                                      //
            GetRootComponent(),                                               //
            NAME_None,                                                        //
            GetActorLocation(),                                               //
            GetActorRotation(),                                               //
            EAttachLocation::KeepWorldPosition,                               //
            false);
    }
}

void AProjectile::ExplodeDamage()
{

    APawn* FiringPawn = GetInstigator();
    if (FiringPawn && FiringPawn->GetController() && HasAuthority())
    {
        AController* FiringController = FiringPawn->GetController();
        UGameplayStatics::ApplyRadialDamageWithFalloff(  //
            this,                                        // World context object
            Damage,                                      // Base damage
            10.f,                                        // Minimum damage
            GetActorLocation(),                          // Origin
            DamageInnerRadius,                           // DamageInnerRadius
            DamageOuterRadius,                           // DamageOuterRadius
            1.f,                                         // DamageFallOff
            UDamageType::StaticClass(),                  // DamageType class
            TArray<AActor*>{},                           // Ignor actors
            this,                                        // Damage causer
            FiringController                             // Instigator Controller
        );
    }
}

void AProjectile::StartDestoryTimer()
{
    GetWorldTimerManager().SetTimer(DestroyTimer, this, &ThisClass::DestroyTimerFinished, DestroyTime, false);
}

void AProjectile::DestroyTimerFinished()
{
    Destroy();
}

void AProjectile::PlayFXAndSound(AActor* OtherActor)
{
    if (OtherActor &&                                   //
        OtherActor->ActorHasTag("BlasterCharacter") &&  //
        OtherActor != GetOwner() &&                     //
        ImpactCharacterParticles &&                     //
        ImpactCharacterSound)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactCharacterParticles, GetActorTransform());
        UGameplayStatics::PlaySoundAtLocation(this, ImpactCharacterSound, GetActorLocation());
    }
    else if (ImpactParticles && ImpactSound)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
        UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
    }
}
