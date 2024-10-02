// Fill out your copyright notice in the Description page of Project Settings.

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "RocketMovementComponent.h"
#include "ProjectileRocket.h"

AProjectileRocket::AProjectileRocket()
{
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>("RocketMesh");
    ProjectileMesh->SetupAttachment(RootComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>("RocketMovementComponent");
    RocketMovementComponent->bRotationFollowsVelocity = true;
    RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
    }

    SpawnTrailSystem();
    if (ProjectileLoop && LoopingSoundAttenuation)
    {
        ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(  //
            ProjectileLoop,                                              //
            GetRootComponent(),                                          //
            NAME_None,                                                   //
            GetActorLocation(),                                          //
            EAttachLocation::KeepWorldPosition,                          //
            false,                                                       //
            1.f,                                                         //
            1.f,                                                         //
            0.f,                                                         //
            LoopingSoundAttenuation,                                     //
            (USoundConcurrency*)nullptr,                                 //
            false);
    }
}

void AProjectileRocket::OnHit(
    UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

    if (OtherActor == GetOwner()) return;

    ExplodeDamage();
    StartDestoryTimer();
    PlayFXAndSound(OtherActor);

    if (ProjectileMesh)
    {
        ProjectileMesh->SetVisibility(false);
    }
    if (CollisionBox)
    {
        CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (TrailSystemComponent)
    {
        TrailSystemComponent->Deactivate();
    }
    if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
    {
        ProjectileLoopComponent->Stop();
    }
}
