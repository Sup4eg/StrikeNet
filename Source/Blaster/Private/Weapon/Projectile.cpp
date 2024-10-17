// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Blaster.h"
#include "Projectile.h"

AProjectile::AProjectile()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

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

    CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
    CollisionBox->IgnoreActorWhenMoving(Owner, true);
    CollisionBox->bReturnMaterialOnMove = true;
    SetLifeSpan(LifeSpan);
}

void AProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AProjectile::OnHit(
    UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    SpawnImpactFXAndSound(Hit);
    Destroy();
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

void AProjectile::SpawnImpactFXAndSound(const FHitResult& FireHit)
{
    FImpactData ImpactData = GetImpactData(FireHit);
    SpawnImpactParticles(FireHit, ImpactData);
    SpawnImpactSound(FireHit, ImpactData);
    SpawnImpactDecal(FireHit, ImpactData);
}

void AProjectile::SpawnImpactParticles(const FHitResult& FireHit, FImpactData& ImpactData)
{
    if (ImpactData.ImpactParticles && GetWorld())
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactData.ImpactParticles, GetActorLocation());
    }
}

void AProjectile::SpawnImpactSound(const FHitResult& FireHit, FImpactData& ImpactData)
{
    if (ImpactData.ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ImpactData.ImpactSound, GetActorLocation());
    }
}

void AProjectile::SpawnImpactDecal(const FHitResult& FireHit, FImpactData& ImpactData)
{
    if (FireHit.bBlockingHit && ImpactData.DecalData.Material && GetWorld())
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

FImpactData AProjectile::GetImpactData(const FHitResult& FireHit)
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
