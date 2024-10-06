// Fill out your copyright notice in the Description page of Project Settings.

#include "Pickup.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "PickupSpawnPoint.h"

APickupSpawnPoint::APickupSpawnPoint()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
    Super::BeginPlay();
    SpawnPickupTimerFinished();
}

void APickupSpawnPoint::SpawnPickup()
{
    uint32 NumPickupClasses = PickupClasses.Num();
    if (NumPickupClasses > 0 && GetWorld())
    {
        uint32 Selection = FMath::RandRange(0, NumPickupClasses - 1);
        SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());

        if (SpawnedPickup && HasAuthority())
        {
            SpawnedPickup->OnDestroyed.AddDynamic(this, &ThisClass::StartSpawnPickupTimer);
        }
    }
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
    const float SpawnTime = FMath::FRandRange(SpawnPickupTimerMin, SpawnPickupTimerMax);
    GetWorldTimerManager().SetTimer(SpawnPickupTimer, this, &ThisClass::SpawnPickupTimerFinished, SpawnTime);
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
    if (HasAuthority())
    {
        SpawnPickup();
    }
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
