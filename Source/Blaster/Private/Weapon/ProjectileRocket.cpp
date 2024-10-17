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
#include "GameFramework/DamageType.h"
#include "LagCompensationComponent.h"
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
}
#endif

void AProjectileRocket::OnHit(
    UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

    if (OtherActor == GetOwner()) return;

    if (ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner()))
    {
        if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
        {
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
                OwnerCharacter->GetController()              // Instigator Controller
            );
        }
        else if (bUseServerSideRewind && OwnerCharacter->GetLagCompensationComponent() && OwnerCharacter->IsLocallyControlled())
        {
            TArray<ABlasterCharacter*> HitCharacters;
            GetHitCharacters(HitCharacters);

            if (!HitCharacters.IsEmpty())
            {
                ABlasterPlayerController* OwnerController = Cast<ABlasterPlayerController>(OwnerCharacter->GetController());
                float HitTime = OwnerController->GetServerTime() - OwnerController->SingleTripTime;
                OwnerCharacter->GetLagCompensationComponent()->ExplosionProjectileServerScoreRequest(  //
                    HitCharacters,                                                                     //
                    TraceStart,                                                                        //
                    InitialVelocity,                                                                   //
                    HitTime,                                                                           //
                    Damage                                                                             //
                );
            }
        }
    }

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

void AProjectileRocket::GetHitCharacters(TArray<ABlasterCharacter*>& OutHitCharacters)
{
    TArray<FHitResult> HitResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());
    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);

    GetWorld()->SweepMultiByObjectType(HitResults, GetActorLocation(), GetActorLocation(), FQuat::Identity, ObjectParams,
        FCollisionShape::MakeSphere(DamageOuterRadius), QueryParams);

    for (FHitResult& HitResult : HitResults)
    {
        if (HitResult.bBlockingHit && HitResult.GetActor() && HitResult.GetActor()->ActorHasTag("BlasterCharacter"))
        {
            if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(HitResult.GetActor()))
            {
                OutHitCharacters.AddUnique(HitCharacter);
            }
        }
    }
}
