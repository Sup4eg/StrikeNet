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
            if (SettingItems.Contains(SettingItemType) && SettingItems[SettingItemType].SettingTitle != FString())
            {
                if (USettingItem* SettingItemWidget = CreateWidget<USettingItem>(this, SettingItemClass))
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