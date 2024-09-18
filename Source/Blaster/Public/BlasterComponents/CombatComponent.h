// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlasterHUD.h"
#include "WeaponTypes.h"
#include "CombatState.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000

struct FHitResult;

class AWeapon;
class ABlasterCharacter;
class ABlasterPlayerController;
class ABlasterHUD;

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

protected:
    virtual void BeginPlay() override;
    void SetAiming(bool bIsAiming);

    UFUNCTION(Server, Reliable)
    void ServerSetAiming(bool bIsAiming);

    UFUNCTION()
    void OnRep_EquippedWeapon();

    UFUNCTION(Server, Reliable)
    void ServerReload();

    void HandleReload();

    int32 GetAmountToReload();

    void FireButtonPressed(bool bPressed);

    UFUNCTION(Server, Reliable)
    void ServerFire(const FVector_NetQuantize& TraceHitTarget);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

    void TraceUnderCrosshairs(FHitResult& TraceHitResult);

    void SetHUDCrosshairs(float DeltaTime);

private:
    float GetCrosshairsSpread(float DeltaTime);
    void InterpFOV(float DeltaTime);
    void Fire();
    void StartFireTimer();
    void FireTimerFinished();
    bool CanFire();
    void InitializeCarriedAmmo();
    void SetCarriedAmmo();
    bool CanReload();
    bool HasEquippedWeaponKey();
    void UpdateAmmoValues();
    void HandleEquipWeapon();

    bool IsControllerValid();

    UFUNCTION()
    void OnRep_CombatState();

    UFUNCTION()
    void OnRep_CarriedAmmo();

    UPROPERTY()
    ABlasterCharacter* Character;

    UPROPERTY()
    ABlasterPlayerController* Controller;

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
    int32 StartingARAmmo = 30;

    TMap<EWeaponType, int32> CarriedAmmoMap;

    UPROPERTY(EditDefaultsOnly)
    TMap<EWeaponType, FName> WeaponTypesToMontageSections;

    UPROPERTY(ReplicatedUsing = OnRep_CombatState)
    ECombatState CombatState = ECombatState::ECS_Unoccupied;
};
