// Fill out your copyright notice in the Description page of Project Settings.

#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Blaster.h"
#include "WeaponTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Pickup.h"

APickup::APickup()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
    OverlapSphere->SetupAttachment(RootComponent);
    OverlapSphere->SetSphereRadius(80.f);
    OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    OverlapSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    OverlapSphere->SetGenerateOverlapEvents(true);
    OverlapSphere->AddLocalOffset(FVector(0.f, 0.f, 85.f));

    PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    PickupMesh->SetupAttachment(OverlapSphere);
    PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PickupMesh->SetRelativeScale3D(FVector(2.5f, 2.5f, 2.5f));

    PickupMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);
    PickupMesh->MarkRenderStateDirty();
    PickupMesh->SetRenderCustomDepth(true);

    PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
    PickupEffectComponent->SetupAttachment(RootComponent);
}

void APickup::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
    }
}

void APickup::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (OverlapSphere)
    {
        OverlapSphere->AddWorldRotation(FRotator(0.f, BaseTurnRate * DeltaTime, 0.f));
    }
}

void APickup::Destroyed()
{
    Super::Destroyed();

    if (PickupSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
    }

    if (PickupEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PickupEffect, GetActorLocation(), GetActorRotation());
    }
}

void APickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
    AActor* OtherActor,                                                  //
    UPrimitiveComponent* OtherComp,                                      //
    int32 OtherBodyIndex,                                                //
    bool bFromSweep,                                                     //
    const FHitResult& SweepResult)
{
}
