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
class UMaterialInterface;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
    EWS_Initial UMETA(DisplayName = "Initial State"),
    EWS_Equipped UMETA(DisplayName = "Equiped"),
    EWS_EquippedSecondary UMETA(DisplayName = "Backpack"),
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

    FVector TraceEndWithScatter(const FVector& HitTarget, const FVector& TraceStart);
    FVector GetTraceStart();

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

    bool bDestroyWeapon = false;

    UPROPERTY(EditAnywhere)
    FName SecondaryWeaponSocketName = "SecondaryWeaponSocket";

    // For Invisibility effect
    UPROPERTY(VisibleAnywhere)
    bool bIsInvisible = false;

    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    bool bUseScatter = false;

protected:
    virtual void BeginPlay() override;
    virtual void OnWeaponStateSet();

    virtual void OnEquipped();
    virtual void OnEquippedSecondary();
    virtual void OnDropped();

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

    /**
     * Trace end with scatter
     */
    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    float DistanceToSphere = 800.f;

    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    float SphereRadius = 75.f;

private:
    UFUNCTION(Client, Reliable)
    void ClientUpdateAmmo(int32 ServerAmmo);

    UFUNCTION(Client, Reliable)
    void ClientAddAmmo(int32 AmmoToAdd);

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

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    int32 Ammo;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    int32 MagCapacity;

    // The number of unprocessed server requests for Ammo
    // Incremented in SpendRound, decremented in ClientUpdateAmmo
    int32 Sequence = 0;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    float AllowedGapToWall = 60.f;

    UPROPERTY()
    ABlasterCharacter* BlasterOwnerCharacter;

    UPROPERTY()
    ABlasterPlayerController* BlasterOwnerController;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    EWeaponType WeaponType;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    EFireType FireType;

    UPROPERTY(VisibleAnywhere);
    TArray<UMaterialInterface*> InitializeMaterials;

    UPROPERTY(EditAnywhere)
    float AimSensitivity = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Sine Parameters")
    float Amplitude = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Sine Parameters")
    float TimeConstant = 5.f;

    float RunningTime;

    float TransformedSin();

    UPROPERTY(EditAnywhere)
    bool bIsHovering = true;

public:
    void SetWeaponState(EWeaponState State);
    bool IsEmpty();
    bool IsFull();

    void SetMaterial(UMaterialInterface* NewMaterial);
    void SetDefaultMaterial();

    FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
    FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; };
    FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; };
    FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; };
    FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; };
    FORCEINLINE EFireType GetWeaponFireType() const { return FireType; };
    FORCEINLINE int32 GetAmmo() const { return Ammo; };
    FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; };
    FORCEINLINE float GetAllowedGapToWall() const { return AllowedGapToWall; };
    FORCEINLINE float GetAimSensitivity() const { return AimSensitivity; };
    FORCEINLINE void SetIsHovering(bool IsHovering) { bIsHovering = IsHovering; };
    FORCEINLINE void SetScatter(bool IsScatter) { bUseScatter = IsScatter; };
};
