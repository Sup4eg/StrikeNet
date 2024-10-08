// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

class USphereComponent;
class USoundBase;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class ABlasterCharacter;

UCLASS()
class BLASTER_API APickup : public AActor
{
    GENERATED_BODY()

public:
    APickup();

    virtual void Tick(float DeltaTime) override;

    UFUNCTION()
    virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
        AActor* OtherActor,                                                 //
        UPrimitiveComponent* OtherComp,                                     //
        int32 OtherBodyIndex,                                               //
        bool bFromSweep,                                                    //
        const FHitResult& SweepResult);

protected:
    virtual void BeginPlay() override;

    virtual void PlayPickupSound();

    bool IsBlasterCharacterValid(AActor* OtherActor);

    UPROPERTY()
    ABlasterCharacter* BlasterCharacter;

private:
    void HandleOverlappingCharacter(AActor* OtherActor);

    UPROPERTY(VisibleAnywhere)
    USphereComponent* OverlapSphere;

    UPROPERTY(EditAnywhere)
    USoundBase* PickupSound;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* PickupMesh;

    UPROPERTY(VisibleAnywhere)
    UNiagaraComponent* PickupEffectComponent;

    UPROPERTY(EditAnywhere)
    UNiagaraSystem* PickupEffect;

    UPROPERTY(EditAnywhere)
    float BaseTurnRate = 45.f;

    FTimerHandle BindOverlapTimer;

    UFUNCTION()
    void BindOverlapTimerFinished();

    float BindOverlapTime = 0.25f;
};
