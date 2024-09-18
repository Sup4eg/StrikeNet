// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Components/SlateWrapperTypes.h"
#include "BlasterPlayerController.generated.h"

class ABlasterHUD;
class UTexture;

UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void OnRep_Pawn() override;

    void SetHUDHealth(float Health, float MaxHealth);
    void SetHUDScore(float Score);
    void SetHUDDefeats(int32 Defeats);
    void SetHUDWeaponAmmo(int32 Ammo);
    void SetHUDCarriedAmmo(int32 Ammo);
    void SetHUDWeaponIcon(UTexture2D* WeaponIcon);
    void HideHUDWeaponAmmoBox();
    void ShowHUDWeaponAmmoBox();    
    void ShowHUDElimmed(const FName& KilledBy);
    void HideHUDElimmed();
    void SetIsDrawHUDCrosshair(bool bIsDrawHUDCrosshair);

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

private:
    bool IsHUDValid();
    bool IsCharacterOverlayValid();

    UPROPERTY()
    ABlasterHUD* BlasterHUD;
};
