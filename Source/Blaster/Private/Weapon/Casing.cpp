// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/StaticMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Casing.h"

ACasing::ACasing()
{
    PrimaryActorTick.bCanEverTick = false;

    CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>("CasingMesh");
    SetRootComponent(CasingMesh);
    CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

    CasingMesh->SetSimulatePhysics(true);
    CasingMesh->SetEnableGravity(true);
    CasingMesh->SetNotifyRigidBodyCollision(true);
    CasingMesh->SetUseCCD(true);
    ShellEjectionImpulse = 5.0f;
}

void ACasing::BeginPlay()
{
    Super::BeginPlay();
    CasingMesh->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
    CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
    SetLifeSpan(3.f);
}

void ACasing::OnHit(
    UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

    if (ShellSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
    }

    CasingMesh->SetNotifyRigidBodyCollision(false);
}