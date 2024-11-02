// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarryItem.h"
#include "Team.h"
#include "Flag.generated.h"

class UNiagaraComponent;

UCLASS()
class BLASTER_API AFlag : public ACarryItem
{
    GENERATED_BODY()

public:
    AFlag();

    virtual void Initialized() override;

protected:
    virtual void BeginPlay() override;

    virtual void OnDropped() override;

    virtual void OnInitialized() override;

    virtual void OnsphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
        AActor* OtherActor,                                                 //
        UPrimitiveComponent* OtherComp,                                     //
        int32 OtherBodyIndex,                                               //
        bool bFromSweep,                                                    //
        const FHitResult& SweepResult) override;

    virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,  //
        AActor* OtherActor,                                                    //
        UPrimitiveComponent* OtherComp,                                        //
        int32 OtherBodyIndex) override;

    virtual void OnEquipped() override;

private:
    void SetReturnToBaseTimer();

    void ReturnToBaseTimerFinished();

    UPROPERTY(VisibleAnywhere)
    USkeletalMeshComponent* CanvasMesh;

    UPROPERTY(EditAnywhere)
    UNiagaraComponent* DropEffect;

    UPROPERTY(EditAnywhere)
    float ReturnToBaseDelay = 120.f;

    UPROPERTY(EditAnywhere)
    ETeam Team;

    FTransform InitialTransform;

    FTimerHandle ReternToBaseTimer;

public:
    FORCEINLINE ETeam GetTeam() const { return Team; }
};
