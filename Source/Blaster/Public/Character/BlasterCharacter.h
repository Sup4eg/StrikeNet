// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TurningInPlace.h"
#include "InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "CombatState.h"
#include "Team.h"
#include "BlasterCharacter.generated.h"

struct FInputActionValue;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UWidgetComponent;
class AWeapon;
class ACarryItem;
class UCombatComponent;
class UBuffComp;
class ULagCompensationComponent;
class ABlasterPlayerController;
class UMaterialInstanceDynamic;
class UMaterialInstance;
class UParticleSystemComponent;
class USoundBase;
class ABlasterPlayerState;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialInterface;
class UBoxComponent;
class UBlasterAnimInstance;
class UPhysicalMaterial;
class ABlasterGameMode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

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

    void PlayFireMontage(bool bAiming);
    void PlayHitReactMontage(AActor* DamageCauser);
    void PlayElimMontage();
    void PlayThrowGrenadeMontage();
    void PlayReloadMontage();
    void PlaySwapWeaponsMontage();
    void PlayMontage(UAnimMontage* Montage, FName SectionName = NAME_None);
    void StopAllMontages();

    UFUNCTION()
    void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

    virtual void OnRep_ReplicatedMovement() override;

    void Elim(bool bPlayerLeftGame);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastElim(bool bPlayerLeftGame);

    virtual void Destroyed() override;

    bool IsInAir();

    UFUNCTION(BlueprintImplementableEvent)
    void ShowSniperScopeWidget(bool bShowScope);

    UFUNCTION(Server, Reliable)
    void ServerSelfDestruction();

    void UpdateHUDHealth();
    void UpdateHUDShield();
    void UpdateHUDAmmo();

    /**
     * Input mapping context
     */
    void SetUpInputMappingContext(UInputMappingContext* MappingContext);

    void SpawnDefaultWeapon();

    bool IsControllerValid();

    bool IsBlasterAnimInstanceValid();

    bool IsBlasterGameModeValid();

    UFUNCTION(Server, Unreliable)
    void ServerUpdateRightHandTransform(const FRotator& NewRightHandRotation);

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

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

    UPROPERTY()
    TMap<FName, UBoxComponent*> HitCollisionBoxes;

    bool bFinishSwapping = true;

    UFUNCTION(Server, Reliable)
    void ServerLeaveGame();

    FOnLeftGame OnLeftGame;

    UFUNCTION(NetMulticast, Reliable)
    void MulticastGainedTheLead();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastLostTheLead();

    bool IsCharacterGainedTheLead();

    UPROPERTY(EditAnywhere, Category = "Player Stats")
    TMap<UPhysicalMaterial*, float> DamageModifiers;

    void SetTeamColor(ETeam Team);

protected:
    virtual void BeginPlay() override;

    void AimOffset(float DeltaTime);
    void RotateInPlace(float DeltaTime);
    void SimProxiesTurn();
    // Poll for any relevan classess and initialize out HUD
    void OnPlayerStateInitialized();
    void PollInit();

    void DropOrDestroyWeapon(AWeapon* Weapon);

    /** Callbacks for input */
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Jump();
    void EquipButtonPressed();
    void CrouchButtonPressed();
    void ReloadButtonPressed();
    void SwapWeaponButtonPressed();
    void AimButtonPressed();
    void AimButtonReleased();
    void FireButtonPressed();
    void FireButtonReleased();
    void ThrowGrenadeButtonPressed();

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputMappingContext* InGameMappingContext;

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

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* SwapWeapon;

    UPROPERTY(Replicated, VisibleInstanceOnly)
    bool bGameplayDisabled = false;

    /** Hit boxes used for server-side rewind */
    UPROPERTY(EditAnywhere)
    UBoxComponent* head;

    UPROPERTY(EditAnywhere)
    UBoxComponent* pelvis;

    UPROPERTY(EditAnywhere)
    UBoxComponent* spine_02;

    UPROPERTY(EditAnywhere)
    UBoxComponent* spine_03;

    UPROPERTY(EditAnywhere)
    UBoxComponent* upperarm_l;

    UPROPERTY(EditAnywhere)
    UBoxComponent* upperarm_r;

    UPROPERTY(EditAnywhere)
    UBoxComponent* lowerarm_l;

    UPROPERTY(EditAnywhere)
    UBoxComponent* lowerarm_r;

    UPROPERTY(EditAnywhere)
    UBoxComponent* hand_l;

    UPROPERTY(EditAnywhere)
    UBoxComponent* hand_r;

    UPROPERTY(EditAnywhere)
    UBoxComponent* backpack;

    UPROPERTY(EditAnywhere)
    UBoxComponent* blanket_l;

    UPROPERTY(EditAnywhere)
    UBoxComponent* blanket_r;

    UPROPERTY(EditAnywhere)
    UBoxComponent* thigh_l;

    UPROPERTY(EditAnywhere)
    UBoxComponent* thigh_r;

    UPROPERTY(EditAnywhere)
    UBoxComponent* calf_l;

    UPROPERTY(EditAnywhere)
    UBoxComponent* calf_r;

    UPROPERTY(EditAnywhere)
    UBoxComponent* foot_l;

    UPROPERTY(EditAnywhere)
    UBoxComponent* foot_r;

private:
    UFUNCTION()
    void OnRep_OverlappingCarryItem(ACarryItem* LastCarryItem);

    UFUNCTION()
    void OnRep_Health();

    UFUNCTION()
    void OnRep_Shield();

    UFUNCTION()
    void OnRep_RightHandTransform();

    UFUNCTION(Server, Reliable)
    void ServerEquipButtonPressed();

    UFUNCTION(Server, Reliable)
    void ServerSwapButtonPressed();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastHitReactMontage(AActor* DamageCauser);

    void HideCharacterIfCameraClose();

    void HideCamera(bool bIsHidden);

    void CalculateAO_Pitch();

    float CalculateSpeed();

    void CheckIfEliminated(AController* InstigatorController);

    UFUNCTION()
    void UpdateDissolveMaterial(float DissolveValue);

    void StartDissolve();

    UFUNCTION()
    void ElimTimerFinished();

    double GetDirectionalHitReactAngle(const FVector& ImpactPoint) const;

    FName GetDirectionalHitReactSection(double Theta) const;

    void SetDynamicDissolveMaterialInstance(float Dissolve, float Glow);

    void SetUpHitShapesSSR();

    UPROPERTY(VisibleAnywhere, Category = Camera)
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, Category = Camera)
    UCameraComponent* FollowCamera;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UWidgetComponent* OverheadWidget;

    /**
     * Blaster components
     */

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UCombatComponent* CombatComp;

    UPROPERTY(VisibleAnywhere)
    UBuffComp* BuffComp;

    UPROPERTY(VisibleAnywhere)
    ULagCompensationComponent* LagCompensationComp;

    UPROPERTY(ReplicatedUsing = OnRep_OverlappingCarryItem, VisibleInstanceOnly)
    ACarryItem* OverlappingCarryItem;

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

    UPROPERTY(EditAnywhere, Category = "Combat")
    UAnimMontage* SwapWeaponsMontage;

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
    UPROPERTY(EditAnywhere, Category = "Player Stats", meta = (ClampMin = 1.f))
    float MaxHealth = 100.f;

    UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
    float Health = 100.f;

    /**
     * Player shield
     */

    UPROPERTY(EditAnywhere, Category = "Player Stats", meta = (ClampMin = 1.f))
    float MaxShield = 100.f;

    UPROPERTY(ReplicatedUsing = OnRep_Shield, EditAnywhere, Category = "Player Stats")
    float Shield = 0.f;

    UPROPERTY()
    ABlasterPlayerController* BlasterPlayerController;

    UPROPERTY()
    ABlasterPlayerState* BlasterPlayerState;

    UPROPERTY()
    UBlasterAnimInstance* BlasterAnimInstance;

    UPROPERTY()
    ABlasterGameMode* BlasterGameMode;

    bool bElimmed = false;

    FTimerHandle ElimTimer;

    UPROPERTY(EditDefaultsOnly)
    float ElimDelay = 3.f;

    /**
     * Dissolve effect
     */

    UPROPERTY(VisibleAnywhere)
    UTimelineComponent* DissolveTimeline;

    UPROPERTY(VisibleAnywhere)
    UTimelineComponent* InvisibilityTimeline;

    FOnTimelineFloat DissolveTrack;

    UPROPERTY(EditAnywhere)
    UCurveFloat* DissolveCurve;

    // dynamic instance that we can change at runtime
    UPROPERTY(VisibleAnywhere, Category = "Elim")
    UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

    UPROPERTY(VisibleAnywhere, Category = "Elim")
    UMaterialInstance* DissolveMaterialInstance;

    // Material instance set on the Blueprint, used with the dynamic material instance
    UPROPERTY(EditDefaultsOnly, Category = "Elim")
    TMap<ETeam, UMaterialInstance*> DissolveMaterialInstanceMap;

    UPROPERTY(EditDefaultsOnly)
    TMap<ETeam, UMaterialInstance*> CharacterMaterialsMap;

    /**
     * Elim effects
     */

    UPROPERTY(EditAnywhere)
    UParticleSystem* ElimBotEffect;

    UPROPERTY(VisibleAnywhere)
    UParticleSystemComponent* ElimBotComponent;

    UPROPERTY(EditAnywhere)
    USoundBase* ElimBotSound;

    UPROPERTY(EditAnywhere)
    UNiagaraSystem* CrownSystem;

    UPROPERTY()
    UNiagaraComponent* CrownComponent;

    /**
     * Grenade
     */
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* AttachedGrenade;

    /**
     * Default Weapon
     */

    UPROPERTY(EditAnywhere)
    TSubclassOf<AWeapon> DefaultWeaponClass;

    // Last pickup effect
    UPROPERTY()
    UNiagaraComponent* PickupEffect;

    UPROPERTY(VisibleAnywhere)
    UMaterialInterface* InitializedMaterial;

    float CurrentSensitivity = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    bool bMoveForward = false;

    UPROPERTY(ReplicatedUsing = OnRep_RightHandTransform)
    FRotator RightHandRotation;

    UPROPERTY()
    UInputMappingContext* LastMappingContext;

    bool bLeftGame = false;

public:
    void SetOverlappingCarryItem(ACarryItem* CarryItem);
    bool IsWeaponEquipped();
    bool IsSecondaryWeapon();
    bool IsFlag();
    bool IsAiming();
    FVector GetHitTarget() const;
    void SetHitTarget(const FVector& NewHitTarget);
    ECombatState GetCombatState() const;
    void SetCombatState(ECombatState NewCombatState);

    AWeapon* GetEquippedWeapon() const;
    AWeapon* GetSecondaryWeapon() const;
    void SetFlag(ACarryItem* FlagToSet);
    ACarryItem* GetFlag() const;
    void SetDefaultMaterial();
    void SetMaterial(UMaterialInterface* NewMaterial);

    bool IsLocallyReloading() const;
    ETeam GetTeam();

    FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; };
    FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; };
    FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; };
    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; };
    FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; };
    FORCEINLINE bool IsElimmed() const { return bElimmed; };
    FORCEINLINE float GetHealth() const { return Health; };
    FORCEINLINE void SetHealth(float Amount) { Health = Amount; };
    FORCEINLINE float GetMaxHealth() const { return MaxHealth; };
    FORCEINLINE float GetShield() const { return Shield; };
    FORCEINLINE void SetShield(float Amount) { Shield = Amount; };
    FORCEINLINE float GetMaxShield() const { return MaxShield; };
    FORCEINLINE bool GetIsElimmed() const { return bElimmed; };
    FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComp; };
    FORCEINLINE UBuffComp* GetBuffComponent() const { return BuffComp; };
    FORCEINLINE ULagCompensationComponent* GetLagCompensationComponent() const { return LagCompensationComp; };
    FORCEINLINE bool GetIsGameplayDisabled() const { return bGameplayDisabled; };
    FORCEINLINE void SetIsGameplayDisabled(bool bDisable) { bGameplayDisabled = bDisable; };
    FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; };
    FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; };
    FORCEINLINE UNiagaraComponent* GetPickupEffect() const { return PickupEffect; };
    FORCEINLINE void SetPickupEffect(UNiagaraComponent* LastPickupEffect) { PickupEffect = LastPickupEffect; };
    FORCEINLINE UTimelineComponent* GetInvisibilityTimeLine() const { return InvisibilityTimeline; };
    FORCEINLINE void SetCurrentSensitivity(float NewSensitivity) { CurrentSensitivity = NewSensitivity; };
    FORCEINLINE ABlasterPlayerController* GetBlasterPlayerController() const { return BlasterPlayerController; };
    FORCEINLINE UInputMappingContext* GetLastMappingContext() const { return LastMappingContext; };
    FORCEINLINE UNiagaraComponent* GetCrownComponent() const { return CrownComponent; };
    FORCEINLINE void SetRightHandRotation(FRotator NewRightHandRotation) { RightHandRotation = NewRightHandRotation; };
};
