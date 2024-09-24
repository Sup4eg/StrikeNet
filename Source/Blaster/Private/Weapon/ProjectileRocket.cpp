// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "RocketMovementComponent.h"
#include "ProjectileRocket.h"

AProjectileRocket::AProjectileRocket()
{
    RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>("RocketMesh");
    RocketMesh->SetupAttachment(RootComponent);
    RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

void AProjectileRocket::DestroyTimerFinished()
{
    Destroy();
}

void AProjectileRocket::OnHit(
    UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

    if (OtherActor == GetOwner()) return;

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

    GetWorldTimerManager().SetTimer(DestroyTimer, this, &ThisClass::DestroyTimerFinished, DestroyTime, false);
    PlayFXAndSound(OtherActor);

    if (RocketMesh)
    {
        RocketMesh->SetVisibility(false);
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
