// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CarryItem.h"
#include "Weapon.generated.h"

class UAnimationAsset;
class ACasing;
class UTexture2D;
class UTexture;

UCLASS()
class BLASTER_API AWeapon : public ACarryItem
{
    GENERATED_BODY()

public:
    AWeapon();

    virtual void Initialized() override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void OnRep_Owner() override;

    void SetHUDAmmo();

    virtual void Fire(const FVector_NetQuantize100& HitTarget, const FVector_NetQuantize100& SocketLocation);

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

    /**
     * Enable / disable custom depth
     */

    void EnableCustomDepth(bool bEnable);

    bool bDestroyWeapon = false;

    UPROPERTY(EditAnywhere)
    FName SecondaryWeaponSocketName = "SecondaryWeaponSocket";

    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    bool bUseScatter = false;

protected:
    virtual void BeginPlay() override;

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

    virtual void OnInitialized() override;

    virtual void OnEquipped() override;
    virtual void OnEquippedSecondary() override;
    virtual void OnDropped() override;

    virtual void OnPingTooHigh(bool bPingTooHigh) override;

    /**
     * Trace end with scatter
     */
    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    float DistanceToSphere = 800.f;

    UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
    float SphereRadius = 75.f;

    UPROPERTY(Replicated, EditAnywhere)
    bool bUseServerSideRewind = false;

    UPROPERTY(EditAnywhere)
    bool bUseServerSideRewindDefault = false;

private:
    UFUNCTION(Client, Reliable)
    void ClientUpdateAmmo(int32 ServerAmmo);

    UFUNCTION(Client, Reliable)
    void ClientAddAmmo(int32 AmmoToAdd);

    void SpendRound();

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

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    EWeaponType WeaponType;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    EFireType FireType;

    UPROPERTY(EditAnywhere)
    float AimSensitivity = 0.5f;

public:
    bool IsEmpty();
    bool IsFull();

    FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; };
    FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; };
    FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; };
    FORCEINLINE EFireType GetWeaponFireType() const { return FireType; };
    FORCEINLINE int32 GetAmmo() const { return Ammo; };
    FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; };
    FORCEINLINE float GetAllowedGapToWall() const { return AllowedGapToWall; };
    FORCEINLINE float GetAimSensitivity() const { return AimSensitivity; };
    FORCEINLINE void SetScatter(bool IsScatter) { bUseScatter = IsScatter; };
};
