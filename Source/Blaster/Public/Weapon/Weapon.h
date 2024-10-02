// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

class USphereComponent;
class UWidgetComponent;
class UAnimationAsset;
class ACasing;
class UTexture2D;
class UTexture;
class ABlasterCharacter;
class ABlasterPlayerController;
class USoundBase;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
    EWS_Initial UMETA(DisplayName = "Initial State"),
    EWS_Equiped UMETA(DisplayName = "Equiped"),
    EWS_Dropped UMETA(DisplayName = "Dropped"),

    EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
    GENERATED_BODY()

public:
    AWeapon();

    virtual void Tick(float DeltaTime) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void OnRep_Owner() override;

    void SetHUDAmmo();

    void ShowPickupWidget(bool bShowWidget);

    virtual void Fire(const FVector& HitTarget);

    void Dropped();

    void AddAmmo(int32 AmmoToAdd);

    /**
     * Textures for the weapon crosshairs
     */

    UPROPERTY(EditAnywhere, Category = "Crosshairs")
    UTexture2D* CrosshairsCenter;

    UPROPERTY(EditAnywhere, Category = "Crosshairs")
    UTexture2D* CrosshairsLeft;

    UPROPERTY(EditAnywhere, Category = "Crosshairs")
    UTexture2D* CrosshairsRight;

    UPROPERTY(EditAnywhere, Category = "Crosshairs")
    UTexture2D* CrosshairsTop;

    UPROPERTY(EditAnywhere, Category = "Crosshairs")
    UTexture2D* CrosshairsBottom;

    /** Icon texture */

    UPROPERTY(EditDefaultsOnly, Category = "Icon")
    UTexture2D* WeaponIcon;

    /**
     * Automatic fire
     */

    UPROPERTY(EditAnywhere, Category = "Combat", meta = (ClampMin = 0.f))
    float FireDelay = .15f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    bool bAutomatic = true;

    UPROPERTY(EditAnywhere, Category = "Weapon properties")
    USoundBase* EquipSound;

    /**
     * Enable / disable custom depth
     */

    void EnableCustomDepth(bool bEnable);

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

private:
    UFUNCTION()
    void OnRep_Ammo();

    void SpendRound();

    bool IsBlasterOwnerCharacterValid();

    UPROPERTY(VisibleAnywhere, Category = "Weapon properties")
    USkeletalMeshComponent* WeaponMesh;

    UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
    USphereComponent* AreaSphere;

    UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
    UWidgetComponent* PickupWidget;

    UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
    EWeaponState WeaponState;

    UFUNCTION()
    void OnRep_WeaponState();

    UPROPERTY(EditAnywhere, Category = "Weapon properties")
    UAnimationAsset* FireAnimation;

    UPROPERTY(EditAnywhere, Category = "Weapon properties")
    TSubclassOf<ACasing> CasingClass;

    /**
     * Zoomed FOV while aiming
     */

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    float ZoomedFOV = 30.f;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    float ZoomInterpSpeed = 20.f;

    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo, Category = "Weapon Properties")
    int32 Ammo;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    int32 MagCapacity;

    UPROPERTY()
    ABlasterCharacter* BlasterOwnerCharacter;

    UPROPERTY()
    ABlasterPlayerController* BlasterOwnerController;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    EWeaponType WeaponType;

public:
    void SetWeaponState(EWeaponState State);
    bool IsEmpty();
    bool IsFull();

    FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
    FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; };
    FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; };
    FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; };
    FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; };
    FORCEINLINE int32 GetAmmo() const { return Ammo; };
    FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; };
};
