
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UProgressBar;
class UTextBlock;
class UElimmedWidget;
class UImage;
class UVerticalBox;
class UHorizontalBox;
class UWidgetAnimation;

UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    UProgressBar* HealthBar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* HealthText;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* ShieldBar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ShieldText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ScoreAmount;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* DefeatsAmount;

    UPROPERTY(meta = (BindWidget))
    UElimmedWidget* ElimmedWidget;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* WeaponAmmoAmount;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* CarriedAmmoAmount;

    UPROPERTY(meta = (BindWidget))
    UImage* WeaponImg;

    UPROPERTY(meta = (BindWidget))
    UVerticalBox* WeaponInfo;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* MatchCountdownText;

    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* GrenadeInfo;

    UPROPERTY(Transient, meta = (BindWidgetAnim))
    UWidgetAnimation* CountdownAnimation;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* GrenadesAmount;

protected:
    virtual void NativeOnInitialized() override;
};
