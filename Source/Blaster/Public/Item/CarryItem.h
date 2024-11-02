// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CarryItemTypes.h"
#include "CarryItem.generated.h"

class USphereComponent;
class UWidgetComponent;
class USkeletalMeshComponent;
class UMaterialInterface;
class ABlasterCharacter;
class ABlasterPlayerController;
class USoundBase;

UCLASS()
class BLASTER_API ACarryItem : public AActor
{
    GENERATED_BODY()

public:
    ACarryItem();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void OnRep_Owner() override;

    void ShowPickupWidget(bool bShowWidget);

    virtual void Tick(float DeltaTime) override;

    void Dropped();

    virtual void Initialized();

    virtual void SetDefaultMaterial();

    virtual void SetMaterial(UMaterialInterface* NewMaterial);

    void SetState(ECarryItemState StateToSet);

    void SetIsHovering(bool IsHovering);

    // For Invisibility effect
    UPROPERTY(VisibleAnywhere)
    bool bIsInvisible = false;

    UPROPERTY(EditAnywhere)
    USoundBase* EquipSound;

    UPROPERTY(EditAnywhere)
    USoundBase* DropSound;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    virtual void OnsphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
        AActor* OtherActor,                                                 //
        UPrimitiveComponent* OtherComp,                                     //
        int32 OtherBodyIndex,                                               //
        bool bFromSweep,                                                    //
        const FHitResult& SweepResult);

    UFUNCTION()
    virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,  //
        AActor* OtherActor,                                                    //
        UPrimitiveComponent* OtherComp,                                        //
        int32 OtherBodyIndex);

    UFUNCTION()
    virtual void OnPingTooHigh(bool bPingTooHigh);

    virtual void OnStateSet();

    virtual void OnInitialized();
    virtual void OnEquipped();
    virtual void OnEquippedSecondary();
    virtual void OnDropped();

    void PlayDropSound();

    bool IsBlasterOwnerCharacterValid();

    bool IsBlasterOwnerControllerValid();

    UPROPERTY(VisibleAnywhere, Category = "Item properties")
    USkeletalMeshComponent* ItemMesh;

    UPROPERTY(VisibleAnywhere, Category = "Item Properties")
    USphereComponent* AreaSphere;

    UPROPERTY(VisibleAnywhere, Category = "Item Properties")
    UWidgetComponent* PickupWidget;

    UPROPERTY(EditAnywhere, Category = "Sine Parameters")
    float Amplitude = 0.2f;

    UPROPERTY(EditAnywhere, Category = "Sine Parameters")
    float TimeConstant = 3.5f;

    float RunningTime;

    float TransformedSin();

    UPROPERTY(EditAnywhere)
    bool bIsHovering = true;

    UPROPERTY()
    ABlasterCharacter* BlasterOwnerCharacter;

    UPROPERTY()
    ABlasterPlayerController* BlasterOwnerController;

private:
    UFUNCTION()
    void OnRep_State();

    UPROPERTY(ReplicatedUsing = OnRep_State, VisibleAnywhere, Category = "Item Properties")
    ECarryItemState State;

    UPROPERTY(VisibleAnywhere);
    TArray<UMaterialInterface*> InitializeMaterials;

public:
    FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
    FORCEINLINE USkeletalMeshComponent* GetItemMesh() const { return ItemMesh; };
};
