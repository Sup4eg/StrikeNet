// Fill out your copyright notice in the Description page of Project Settings.

#include "ElimmedWidget.h"
#include "Components/VerticalBox.h"
#include "CharacterOverlay.h"

void UCharacterOverlay::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (ElimmedWidget)
    {
        ElimmedWidget->SetVisibility(ESlateVisibility::Collapsed);
        WeaponBox->SetVisibility(ESlateVisibility::Collapsed);
    }
}
