#include "BlasterPlayerController.h"
// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterHUD.h"
#include "CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.h"
#include "ElimmedWidget.h"
#include "BlasterCharacter.h"
#include "BlasterUtils.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "BlasterPlayerController.h"

void ABlasterPlayerController::BeginPlay()
{
    Super::BeginPlay();
    BlasterHUD = Cast<ABlasterHUD>(GetHUD());
}

void ABlasterPlayerController::OnRep_Pawn()
{
    Super::OnRep_Pawn();
    HideHUDElimmed();
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
    if (BlasterCharacter)
    {
        SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
        HideHUDElimmed();
    }
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                //
                     BlasterHUD->CharacterOverlay->HealthBar &&  //
                     BlasterHUD->CharacterOverlay->HealthText;
    if (!bHUDValid) return;
    const float HealthPercent = Health / MaxHealth;
    const FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
    BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
    BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
    bool bHUDValid = IsCharacterOverlayValid() && BlasterHUD->CharacterOverlay->ScoreAmount;
    if (!bHUDValid) return;
    FString ScoreText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Score));
    BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
    bool bHUDValid = IsCharacterOverlayValid() && BlasterHUD->CharacterOverlay->DefeatsAmount;
    if (!bHUDValid) return;
    FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
    BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
}

void ABlasterPlayerController::ShowHUDElimmed(const FName& KilledBy)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                    //
                     BlasterHUD->CharacterOverlay->ElimmedWidget &&  //
                     BlasterHUD->CharacterOverlay->ElimmedWidget->KilledBy;

    if (!bHUDValid) return;
    BlasterHUD->CharacterOverlay->ElimmedWidget->KilledBy->SetText(FText::FromName(KilledBy));
    BlasterHUD->CharacterOverlay->ElimmedWidget->SetVisibility(ESlateVisibility::Visible);
}

void ABlasterPlayerController::HideHUDElimmed()
{
    bool bHUDValid = IsCharacterOverlayValid() &&                    //
                     BlasterHUD->CharacterOverlay->ElimmedWidget &&  //
                     BlasterHUD->CharacterOverlay->ElimmedWidget->KilledBy;

    if (!bHUDValid) return;
    BlasterHUD->CharacterOverlay->ElimmedWidget->KilledBy->SetText(FText());
    BlasterHUD->CharacterOverlay->ElimmedWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                     //
                     BlasterHUD->CharacterOverlay->WeaponAmmoAmount;  //

    if (!bHUDValid) return;
    FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
    BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                      //
                     BlasterHUD->CharacterOverlay->CarriedAmmoAmount;  //

    if (!bHUDValid) return;
    FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
    BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
}

void ABlasterPlayerController::SetHUDWeaponIcon(UTexture2D* WeaponIcon)
{

    bool bHUDValid = IsCharacterOverlayValid() &&              //
                     BlasterHUD->CharacterOverlay->WeaponImg;  //
    if (!bHUDValid || !WeaponIcon) return;
    BlasterHUD->CharacterOverlay->WeaponImg->SetBrushFromTexture(WeaponIcon);
}

void ABlasterPlayerController::HideHUDWeaponAmmoBox()
{
    bool bHUDValid = IsCharacterOverlayValid() &&              //
                     BlasterHUD->CharacterOverlay->WeaponBox;  //
    if (!bHUDValid) return;
    BlasterHUD->CharacterOverlay->WeaponBox->SetVisibility(ESlateVisibility::Collapsed);
}

void ABlasterPlayerController::ShowHUDWeaponAmmoBox()
{
    bool bHUDValid = IsCharacterOverlayValid() &&              //
                     BlasterHUD->CharacterOverlay->WeaponBox;  //
    if (!bHUDValid) return;
    BlasterHUD->CharacterOverlay->WeaponBox->SetVisibility(ESlateVisibility::Visible);
}

void ABlasterPlayerController::SetIsDrawHUDCrosshair(bool bIsDrawHUDCrosshair)
{
    if (IsHUDValid())
    {
        BlasterHUD->SetIsDrawCrosshair(bIsDrawHUDCrosshair);
    }
}

bool ABlasterPlayerController::IsHUDValid()
{
    return BlasterUtils::CastOrUseExistsActor<ABlasterHUD>(BlasterHUD, GetHUD());
}

bool ABlasterPlayerController::IsCharacterOverlayValid()
{
    return IsHUDValid() && BlasterHUD->CharacterOverlay;
}
