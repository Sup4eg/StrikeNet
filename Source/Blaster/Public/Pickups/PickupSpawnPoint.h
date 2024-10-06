// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class APickup;

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
    GENERATED_BODY()

public:
    APickupSpawnPoint();
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere)
    TArray<TSubclassOf<APickup>> PickupClasses;

    UPROPERTY()
    APickup* SpawnedPickup;

    void SpawnPickup();

    void SpawnPickupTimerFinished();

    UFUNCTION()
    void StartSpawnPickupTimer(AActor* DestroyedActor);

private:
    FTimerHandle SpawnPickupTimer;

    UPROPERTY(EditAnywhere, meta = (ClampMin = 0.f))
    float SpawnPickupTimerMin = 5.f;

    UPROPERTY(EditAnywhere, meta = (ClampMin = 0.f))
    float SpawnPickupTimerMax = 10.f;
};
