// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuTypes/SettingTypes.h"
#include "SettingItem.h"
#include "Components/VerticalBox.h"
#include "BaseSettingsTab.h"

bool UBaseSettingsTab::Initialize()
{
    bool Result = Super::Initialize();
    if (SettingItemClass && SettingsBox)
    {
        for (ESettingType& SettingItemType : SettingsItemOrder)
        {
            if (SettingItems.Contains(SettingItemType) && SettingItems[SettingItemType].SettingTitle != FString() && GetWorld())
            {
                if (USettingItem* SettingItemWidget = CreateWidget<USettingItem>(GetWorld(), SettingItemClass))
                {
                    SettingItemWidget->SetData(SettingItems[SettingItemType].SettingTitle, SettingItemType, this);
                    if (SettingsBox)
                    {
                        SettingsBox->AddChild(SettingItemWidget);
                    }
                }
            }
        }
    }
    return Result;
}