// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TurningInPlace.h"
#include "InteractWithCrosshairsInterface.h"
#include "BlasterCharacter.generated.h"

struct FInputActionValue;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UWidgetComponent;
class AWeapon;
class UCombatComponent;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
    GENERATED_BODY()

public:
    ABlasterCharacter();

    virtual void Tick(float DeltaTime) override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void PostInitializeComponents() override;

    void PlayFireMontage(bool bAiming);

    void PlayHitReactMontage();

    virtual void OnRep_ReplicatedMovement() override;

    bool IsInAir();

protected:
    virtual void BeginPlay() override;

    /** Input */
    void SetUpInputMappingContext();

    /** Callbacks for input */
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Jump();
    void EquipButtonPressed();
    void CrouchButtonPressed();
    void AimButtonPressed();
    void AimButtonReleased();
    void FireButtonPressed();
    void FireButtonReleased();
    void AimOffset(float DeltaTime);
    void SimProxiesTurn();

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputMappingContext* DefaultMappingContext;

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

private:
    UFUNCTION()
    void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

    UFUNCTION(Server, Reliable)
    void ServerEquipButtonPressed();

    void HideCameraIfCharacterClose();

    void HideCamera(bool bIsHidden);

    void CalculateAO_Pitch();

    float CalculateSpeed();

    UPROPERTY(VisibleAnywhere, Category = Camera)
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, Category = Camera)
    UCameraComponent* FollowCamera;

    UPROPERTY(VisibleAnywhere)
    UCombatComponent* CombatComp;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UWidgetComponent* OverheadWidget;

    UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
    AWeapon* OverlappingWeapon;

    float AO_Yaw;

    float InterpAO_Yaw;

    float AO_Pitch;

    FRotator StartingAimRotation;

    ETurningInPlace TurningInPlace;
    void TurnInPlace(float DeltaTime);

    UPROPERTY(EditAnywhere, Category = "Combat")
    UAnimMontage* FireWeaponMontage;

    UPROPERTY(EditAnywhere, Category = "Combat")
    UAnimMontage* HitReactMontage;

    UPROPERTY(EditAnywhere)
    float CameraThreshold = 200.f;

    bool bRotateRootBone;
    float TurnThreshold = 5.f;
    FRotator ProxyRotationLastFrame;
    FRotator ProxyRotation;
    float ProxyYaw;
    float TimeSinceLastMovementReplication;

public:
    void SetOverlappingWeapon(AWeapon* Weapon);
    bool IsWeaponEquipped();
    bool IsAiming();
    AWeapon* GetEquippedWeapon();
    FVector GetHitTarget() const;

    FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; };
    FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; };
    FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; };
    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; };
    FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; };
};
