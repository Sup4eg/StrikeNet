// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlasterHUD.h"
#include "CarryItemTypes.h"
#include "CombatState.h"
#include "CombatComponent.generated.h"

struct FHitResult;

class AWeapon;
class ABlasterCharacter;
class ABlasterPlayerController;
class ABlasterHUD;
class AProjectile;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLASTER_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();
    friend class ABlasterCharacter;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void EquipWeapon(AWeapon* WeaponToEquip);
    void SwapWeapons();
    void Reload();

    UFUNCTION(BlueprintCallable)
    void FinishReloading();

    UFUNCTION(BlueprintCallable)
    void FinishSwapWeapons();

    UFUNCTION(BlueprintCallable)
    void FinishSwapAttachWeapons();

    void SetAiming(bool bIsAiming);

    void FireButtonPressed(bool bPressed);

    UFUNCTION(BlueprintCallable)
    void ShotgunShellReload();

    UFUNCTION(BlueprintCallable)
    void ThrowGrenadeFinished();

    UFUNCTION(BlueprintCallable)
    void LaunchGrenade();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastLaunchGrenade(const FVector_NetQuantize& Target);

    UFUNCTION(Server, Reliable)
    void ServerLaunchGrenade(const FVector_NetQuantize& Target);

    void PickupAmmo(EWeaponType WeaponType, uint32 AmmoAmount);

    void CachePendingSwapWeapons();

    bool bLocallyReloading = false;

protected:
    virtual void BeginPlay() override;

    UFUNCTION(Server, Reliable)
    void ServerSetAiming(bool bIsAiming);

    UFUNCTION()
    void OnRep_EquippedWeapon(AWeapon* LastEquippedWeapon);

    UFUNCTION()
    void OnRep_SecondaryWeapon(AWeapon* LastSecondaryWeapon);

    UFUNCTION(Server, Reliable)
    void ServerReload();

    void HandleReload();

    int32 GetAmountToReload();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerFire(const FVector_NetQuantize100& TraceHitTarget, const FVector_NetQuantize100& SocketLocation, float FireDelay);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastFire(const FVector_NetQuantize100& TraceHitTarget, const FVector_NetQuantize100& SocketLocation);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerShotgunFire(
        const TArray<FVector_NetQuantize100>& TraceHitTargets, const FVector_NetQuantize100& SocketLocation, float FireDelay);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastShotgunFire(const TArray<FVector_NetQuantize100>& TraceHitTargets, const FVector_NetQuantize100& SocketLocation);

    void TraceUnderCrosshairs(FHitResult& TraceHitResult);

    void SetHUDCrosshairs(float DeltaTime);

    void ThrowGrenade();

    UFUNCTION(Server, Reliable)
    void ServerThrowGrenade();

    void DropEquippedWeapon();
    void AttachWeaponToRightHand(AWeapon* WeaponToAttach);
    void AttachWeaponToLeftHand(AWeapon* WeaponToAttach);
    void AttachWeaponToBackpack(AWeapon* WeaponToAttach);
    void SetCarriedAmmo();
    void PlayEquipWeaponSound(AWeapon* WeaponToEquip);
    void ReloadEmptyWeapon();
    void ShowAttachedGrenade(bool bShowAttachedGrenade);
    void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
    void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

    UPROPERTY(EditAnywhere)
    TSubclassOf<AProjectile> GrenadeClass;

private:
    float GetCrosshairsSpread(float DeltaTime);
    void InterpFOV(float DeltaTime);
    void Fire();
    void FireProjectileWeapon();
    void FireHitScanWeapon();
    void FireShotgun();
    void LocalFire(const FVector_NetQuantize100& TraceHitTarget, const FVector_NetQuantize100& SocketLocation);
    void ShotgunLocalFire(const TArray<FVector_NetQuantize100>& TraceHitTargets, const FVector_NetQuantize100& SocketLocation);
    void StartFireTimer();
    void FireTimerFinished();
    bool CanFire();
    void InitializeCarriedAmmo();
    bool CanReload();
    bool HasEquippedWeaponKey();
    void UpdateAmmoValues();
    void UpdateShotgunAmmoValues();
    void HandleWeaponSpecificLogic(AWeapon* LastWeapon, AWeapon* NewWeapon);

    void SetSecondaryWeaponInvisible();
    void SetEquippedWeaponInvisible();

    bool IsControllerValid();

    bool IsInvisibilityActive() const;

    bool IsCloseToWall();

    FVector GetWeaponSocketLocation();

    /**
     * Rep notifies
     */

    UFUNCTION() void OnRep_CombatState(ECombatState LastCombatState);

    UFUNCTION()
    void OnRep_CarriedAmmo();

    UFUNCTION()
    void OnRep_Grenades();

    UFUNCTION()
    void OnRep_Aiming();

    void UpdateHUDGrenades();

    void SpawnGrenade(const FVector_NetQuantize& Target);

    UPROPERTY()
    ABlasterCharacter* BlasterCharacter;

    UPROPERTY()
    ABlasterPlayerController* BlasterController;

    UPROPERTY()
    ABlasterHUD* HUD;

    UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
    AWeapon* EquippedWeapon;

    UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
    AWeapon* SecondaryWeapon;

    UPROPERTY(Transient)
    AWeapon* PendingSwapEquippedWeapon;

    UPROPERTY(Transient)
    AWeapon* PendingSwapSecondaryWeapon;

    UPROPERTY(ReplicatedUsing = OnRep_Aiming)
    bool bAiming = false;

    bool bAimButtonPressed = false;

    bool bFireButtonPressed;

    /**
     * HUD and crosshairs
     */
    float CrosshairVelocityFactor;
    float CrosshairInAirFactor;
    float CrosshairAimFactor;
    float CrosshairCharacterFactor;
    float CrosshairShootingFactor;

    FVector HitTarget;

    FHUDPackage HUDPackage;

    /**
     * Aiming and FOV
     */
    // Field of view when not aiming; set to the camera's FOV in BeginPlay
    float DefaultFOV;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float ZoomedFOV = 30.f;

    float CurrentFOV;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float ZoomInterpSpeed = 20.f;

    FTimerHandle FireTimer;
    bool bCanFire = true;

    // Carried ammo for the currently-equipped weapon
    UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
    int32 CarriedAmmo;

    UPROPERTY(EditAnywhere)
    int32 StartingARAmmo = 60;

    UPROPERTY(EditAnywhere)
    int32 StartingRocketAmmo = 8;

    UPROPERTY(EditAnywhere)
    int32 StartingPistolAmmo = 30;

    UPROPERTY(EditAnywhere)
    int32 StartingSMGAmmo = 60;

    UPROPERTY(EditAnywhere)
    int32 StartingShotgunAmmo = 10;

    UPROPERTY(EditAnywhere)
    int32 StartingSniperRifleAmmo = 6;

    UPROPERTY(EditAnywhere)
    int32 StartingGrenadeLauncherAmmo = 8;

    UPROPERTY(ReplicatedUsing = OnRep_Grenades)
    int32 Grenades = 4;

    UPROPERTY(EditAnywhere, meta = (ClampMin = "4"))
    int32 StartingGrenades = 4;

    UPROPERTY(EditDefaultsOnly)
    uint32 MaxAmmo = 500;

    TMap<EWeaponType, int32> CarriedAmmoMap;

    UPROPERTY(EditDefaultsOnly)
    TMap<EWeaponType, FName> WeaponTypesToMontageSections;

    UPROPERTY(ReplicatedUsing = OnRep_CombatState)
    ECombatState CombatState = ECombatState::ECS_Unoccupied;

    bool bHoldingTheFlag = false;

public:
    bool ShouldSwapWeapons() const;
};
