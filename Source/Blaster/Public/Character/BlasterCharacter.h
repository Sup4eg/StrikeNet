// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TurningInPlace.h"
#include "InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "CombatState.h"
#include "BlasterCharacter.generated.h"

struct FInputActionValue;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UWidgetComponent;
class AWeapon;
class UCombatComponent;
class UBuffComponent;
class ABlasterPlayerController;
class UMaterialInstanceDynamic;
class UMaterialInstance;
class UParticleSystemComponent;
class USoundBase;
class ABlasterPlayerState;
class UStaticMeshComponent;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
    GENERATED_BODY()

public:
    ABlasterCharacter();

    virtual void Tick(float DeltaTime) override;

    virtual void PossessedBy(AController* NewController) override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void PostInitializeComponents() override;

    virtual void Landed(const FHitResult& Hit) override;

    void PlayFireMontage(bool bAiming);
    void PlayHitReactMontage(AActor* DamageCauser);
    void PlayElimMontage();
    void PlayThrowGrenadeMontage();
    void PlayReloadMontage();
    void PlayMontage(UAnimMontage* Montage, FName SectionName = NAME_None);
    void StopAllMontages();

    UFUNCTION()
    void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

    virtual void OnRep_ReplicatedMovement() override;

    void Elim();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastElim();

    virtual void Destroyed() override;

    bool IsInAir();

    UFUNCTION(BlueprintImplementableEvent)
    void ShowSniperScopeWidget(bool bShowScope);

    void UpdateHUDHealth();

    /**
     * Input mapping context
     */
    void SetUpInputMappingContext(UInputMappingContext* MappingContext);

    bool bDrawCrosshair = true;

    /**
     * Movement properties
     */

    UPROPERTY(EditAnywhere, Category = "Movement")
    float BaseWalkSpeed = 600.f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float CrouchWalkSpeed = 300.f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float AimWalkSpeed = 450.f;

protected:
    virtual void BeginPlay() override;

    void AimOffset(float DeltaTime);
    void RotateInPlace(float DeltaTime);
    void SimProxiesTurn();
    // Poll for any relevan classess and initialize out HUD
    void PollInit();

    /** Callbacks for input */
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Jump();
    void EquipButtonPressed();
    void CrouchButtonPressed();
    void ReloadButtonPressed();
    void AimButtonPressed();
    void AimButtonReleased();
    void FireButtonPressed();
    void FireButtonReleased();
    void ThrowGrenadeButtonPressed();

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputMappingContext* ElimmedMappingContext;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* EquipAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* CrouchAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* AimAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* FireAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* ReloadAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* ThrowGrenade;

    UPROPERTY(Replicated, VisibleInstanceOnly)
    bool bGameplayDisabled = false;

private:
    UFUNCTION()
    void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

    UFUNCTION()
    void OnRep_Health();

    UFUNCTION(Server, Reliable)
    void ServerEquipButtonPressed();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastHitReactMontage(AActor* DamageCauser);

    void HideCameraIfCharacterClose();

    void HideCamera(bool bIsHidden);

    void CalculateAO_Pitch();

    float CalculateSpeed();

    void CheckIfEliminated(AController* InstigatorController);

    UFUNCTION()
    void UpdateDissolveMaterial(float DissolveValue);

    void StartDissolve();

    UFUNCTION()
    void ElimTimerFinished();

    bool IsControllerValid();

    bool IsHideSniperScope();

    double GetDirectionalHitReactAngle(const FVector& ImpactPoint) const;

    FName GetDirectionalHitReactSection(double Theta) const;

    UPROPERTY(VisibleAnywhere, Category = Camera)
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, Category = Camera)
    UCameraComponent* FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UCombatComponent* CombatComp;

    UPROPERTY(VisibleAnywhere)
    UBuffComponent* BuffComp;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UWidgetComponent* OverheadWidget;

    UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon, VisibleInstanceOnly)
    AWeapon* OverlappingWeapon;

    float AO_Yaw;

    float InterpAO_Yaw;

    float AO_Pitch;

    FRotator StartingAimRotation;

    ETurningInPlace TurningInPlace;
    void TurnInPlace(float DeltaTime);

    /**
     * Animation montages
     */

    UPROPERTY(EditAnywhere, Category = "Combat")
    UAnimMontage* FireWeaponMontage;

    UPROPERTY(EditAnywhere, Category = "Combat")
    UAnimMontage* HitReactMontage;

    UPROPERTY(EditAnywhere, Category = "Combat")
    UAnimMontage* ElimMontage;

    UPROPERTY(EditAnywhere, Category = "Combat")
    UAnimMontage* ReloadMontage;

    UPROPERTY(EditAnywhere, Category = "Combat")
    UAnimMontage* ThrowGrenadeMontage;

    UPROPERTY(EditAnywhere)
    float CameraThreshold = 200.f;

    bool bRotateRootBone;
    float TurnThreshold = 5.f;
    FRotator ProxyRotationLastFrame;
    FRotator ProxyRotation;
    float ProxyYaw;
    float TimeSinceLastMovementReplication;

    /**
     * Player health
     */
    UPROPERTY(EditAnywhere, Category = "Player Stats")
    float MaxHealth = 100.f;

    UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
    float Health = 100.f;

    UPROPERTY()
    ABlasterPlayerController* BlasterPlayerController;

    bool bElimmed = false;

    FTimerHandle ElimTimer;

    UPROPERTY(EditDefaultsOnly)
    float ElimDelay = 3.f;

    /**
     * Dissolve effect
     */

    UPROPERTY(VisibleAnywhere)
    UTimelineComponent* DissolveTimeline;

    FOnTimelineFloat DissolveTrack;

    UPROPERTY(EditAnywhere)
    UCurveFloat* DissolveCurve;

    // dynamic instance that we can change at runtime
    UPROPERTY(VisibleAnywhere, Category = "Elim")
    UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

    // Material instance set on the Blueprint, used with the dynamic material instance
    UPROPERTY(EditAnywhere, Category = "Elim")
    UMaterialInstance* DissolveMaterialInstance;

    /**
     * Elim bot
     */

    UPROPERTY(EditAnywhere)
    UParticleSystem* ElimBotEffect;

    UPROPERTY(VisibleAnywhere)
    UParticleSystemComponent* ElimBotComponent;

    UPROPERTY(EditAnywhere)
    USoundBase* ElimBotSound;

    UPROPERTY()
    ABlasterPlayerState* BlasterPlayerState;

    /**
     * Grenade
     */
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* AttachedGrenade;

public:
    void SetOverlappingWeapon(AWeapon* Weapon);
    bool IsWeaponEquipped();
    bool IsAiming();
    AWeapon* GetEquippedWeapon();
    FVector GetHitTarget() const;
    ECombatState GetCombatState() const;

    FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; };
    FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; };
    FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; };
    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; };
    FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; };
    FORCEINLINE bool IsElimmed() const { return bElimmed; };
    FORCEINLINE float GetHealth() const { return Health; };
    FORCEINLINE void SetHealth(float Amount) { Health = Amount; };
    FORCEINLINE float GetMaxHealth() const { return MaxHealth; };
    FORCEINLINE bool GetIsElimmed() const { return bElimmed; };
    FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComp; };
    FORCEINLINE UBuffComponent* GetBuffComponent() const { return BuffComp; };
    FORCEINLINE bool GetIsGameplayDisabled() const { return bGameplayDisabled; };
    FORCEINLINE void SetIsGameplayDisabled(bool bDisable) { bGameplayDisabled = bDisable; };
    FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; };
    FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; };
};
