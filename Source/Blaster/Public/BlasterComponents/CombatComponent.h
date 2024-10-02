// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlasterHUD.h"
#include "WeaponTypes.h"
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
    void Reload();

    UFUNCTION(BlueprintCallable)
    void FinishReloading();

    void SetAiming(bool bIsAiming);

    void FireButtonPressed(bool bPressed);

    UFUNCTION(BlueprintCallable)
    void ShotgunShellReload();

    UFUNCTION(BlueprintCallable)
    void ThrowGrenadeFinished();

    UFUNCTION(BlueprintCallable)
    void LaunchGrenade();

    UFUNCTION(Server, Reliable)
    void ServerLaunchGranade(const FVector_NetQuantize Target);

    void PickupAmmo(EWeaponType WeaponType, uint32 AmmoAmount);

protected:
    virtual void BeginPlay() override;

    UFUNCTION(Server, Reliable)
    void ServerSetAiming(bool bIsAiming);

    UFUNCTION()
    void OnRep_EquippedWeapon(AWeapon* LastEquippedWeapon);

    UFUNCTION(Server, Reliable)
    void ServerReload();

    void HandleReload();

    int32 GetAmountToReload();

    UFUNCTION(Server, Reliable)
    void ServerFire(const FVector_NetQuantize& TraceHitTarget);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

    void TraceUnderCrosshairs(FHitResult& TraceHitResult);

    void SetHUDCrosshairs(float DeltaTime);

    void ThrowGrenade();

    UFUNCTION(Server, Reliable)
    void ServerThrowGrenade();

    void DropEquippedWeapon();
    void AttachWeaponToRightHand(AWeapon* WeaponToAttach);
    void AttachWeaponToLeftHand(AWeapon* WeaponToAttach);
    void SetCarriedAmmo();
    void PlayEquipWeaponSound();
    void ReloadEmptyWeapon();
    void ShowAttachedGrenade(bool bShowAttachedGrenade);

    UPROPERTY(EditAnywhere)
    TSubclassOf<AProjectile> GrenadeClass;

private:
    float GetCrosshairsSpread(float DeltaTime);
    void InterpFOV(float DeltaTime);
    void Fire();
    void StartFireTimer();
    void FireTimerFinished();
    bool CanFire();
    void InitializeCarriedAmmo();
    bool CanReload();
    bool HasEquippedWeaponKey();
    void UpdateAmmoValues();
    void UpdateShotgunAmmoValues();
    void HandleWeaponSpecificLogic(AWeapon* LastWeapon, AWeapon* NewWeapon);

    bool IsControllerValid();

    /**
     * Rep notifies
     */

    UFUNCTION()
    void OnRep_CombatState();

    UFUNCTION()
    void OnRep_CarriedAmmo();

    UFUNCTION()
    void OnRep_Grenades();

    void UpdateHUDGrenades();

    UPROPERTY()
    ABlasterCharacter* BlasterCharacter;

    UPROPERTY()
    ABlasterPlayerController* BlasterController;

    UPROPERTY()
    ABlasterHUD* HUD;

    UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_EquippedWeapon)
    AWeapon* EquippedWeapon;

    UPROPERTY(Replicated)
    bool bAiming;

    UPROPERTY(EditAnywhere)
    float BaseWalkSpeed;

    UPROPERTY(EditAnywhere)
    float AimWalkSpeed;

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
    int32 MaxARAmmo = 85;

    UPROPERTY(EditAnywhere)
    int32 MaxRocketAmmo = 12;

    UPROPERTY(EditAnywhere)
    int32 MaxPistolAmmo = 42;

    UPROPERTY(EditAnywhere)
    int32 MaxSMGAmmo = 85;

    UPROPERTY(EditAnywhere)
    int32 MaxShotgunAmmo = 12;

    UPROPERTY(EditAnywhere)
    int32 MaxSniperRifleAmmo = 12;

    UPROPERTY(EditAnywhere)
    int32 MaxGrenadeLauncherAmmo = 12;

    UPROPERTY(ReplicatedUsing = OnRep_Grenades)
    int32 Grenades = 4;

    UPROPERTY(EditAnywhere, meta = (ClampMin = "4"))
    int32 MaxGrenades = 4;

    TMap<EWeaponType, int32> CarriedAmmoMap;
    TMap<EWeaponType, int32> MaxAmmoMap;

    UPROPERTY(EditDefaultsOnly)
    TMap<EWeaponType, FName> WeaponTypesToMontageSections;

    UPROPERTY(ReplicatedUsing = OnRep_CombatState)
    ECombatState CombatState = ECombatState::ECS_Unoccupied;
};
