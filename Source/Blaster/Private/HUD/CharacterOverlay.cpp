// Fill out your copyright notice in the Description page of Project Settings.

#include "ElimmedWidget.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "CharacterOverlay.h"

void UCharacterOverlay::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (ElimmedWidget)
    {
        ElimmedWidget->SetVisibility(ESlateVisibility::Collapsed);
        WeaponInfo->SetVisibility(ESlateVisibility::Collapsed);
        GrenadeInfo->SetVisibility(ESlateVisibility::Collapsed);
    }
}
