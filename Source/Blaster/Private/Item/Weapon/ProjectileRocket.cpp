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
#include "Engine/World.h"
#include "BlasterCharacter.h"
#include "BlasterPlayerController.h"
#include "LagCompensationComponent.h"
#include "Weapon.h"
#include "BlasterGameplayStatics.h"
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

    // Debug purpose

    // FPredictProjectilePathParams PathParams;
    // PathParams.bTraceWithChannel = true;
    // PathParams.bTraceWithCollision = true;
    // PathParams.MaxSimTime = 4.f;
    // PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
    // PathParams.OverrideGravityZ = GetWorld()->GetGravityZ() * RocketMovementComponent->ProjectileGravityScale;
    // PathParams.StartLocation = GetActorLocation();
    // PathParams.SimFrequency = 15.f;
    // PathParams.ProjectileRadius = 5.f;
    // PathParams.TraceChannel = ECC_Visibility;
    // PathParams.ActorsToIgnore.Add(this);
    // PathParams.DrawDebugTime = 5.f;
    // PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;

    // FPredictProjectilePathResult PathResult;
    // UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
}

#if WITH_EDITOR
void AProjectileRocket::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);

    FName PropertyName = Event.Property ? Event.Property->GetFName() : NAME_None;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileRocket, InitialSpeed) && RocketMovementComponent)
    {
        RocketMovementComponent->InitialSpeed = InitialSpeed;
        RocketMovementComponent->MaxSpeed = InitialSpeed;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileRocket, GravityScale) && RocketMovementComponent)
    {
        RocketMovementComponent->ProjectileGravityScale = GravityScale;
    }
}
#endif

void AProjectileRocket::OnHit(
    UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

    if (OtherActor == GetOwner()) return;

    ExplodeDamage(Hit.ImpactPoint);
    StartDestoryTimer();
    SpawnImpactFXAndSound(Hit);

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