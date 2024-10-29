// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "BaseSettingsTab.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "SettingItem.h"

bool USettingItem::Initialize()
{
    bool Result = Super::Initialize();
    if (SettingButton)
    {
        SettingButton->OnClicked.AddDynamic(this, &ThisClass::SettingButtonClicked);
        SettingButton->OnHovered.AddDynamic(this, &ThisClass::SettingButtonHovered);
        SettingButton->OnUnhovered.AddDynamic(this, &ThisClass::SettingButtonUnHovered);
    }
    return Result;
}

void USettingItem::SetData(FString NewSettingTitle, ESettingType NewSettingType, UBaseSettingsTab* TabToSet)
{
    if (SettingTitle)
    {
        SettingTitle->SetText(FText::FromString(NewSettingTitle));
    }
    SettingsTab = TabToSet;
    Type = NewSettingType;
    SetSettingValue(GetSettingValue());
}

void USettingItem::SettingButtonClicked()
{
    if (!IsTabValid()) return;
    FString SettingValueStr = FString();
    switch (Type)
    {
        // GeneralSettings
        case ESettingType::ST_WindowMode: SetWindowModeValueStr(SettingValueStr); break;
        case ESettingType::ST_Resolution: SetResolutionValueStr(SettingValueStr); break;
        case ESettingType::ST_Vsync: SetVsyncValueStr(SettingValueStr); break;

        // GraphycSettings
        case ESettingType::ST_OverallGraphycs: SetOverallGraphycsValueStr(SettingValueStr); break;
        case ESettingType::ST_ViewDistance: SetViewDistanceValueStr(SettingValueStr); break;
        case ESettingType::ST_PostProcessing: SetPostProcessingValueStr(SettingValueStr); break;
        case ESettingType::ST_GlobalIllumination: SetGlobalIlluminationValueStr(SettingValueStr); break;
        case ESettingType::ST_Textures: SetTexturesValueStr(SettingValueStr); break;
        case ESettingType::ST_VisualEffects: SetVisualEffectsValueStr(SettingValueStr); break;
        default: break;
    }
    SetSettingValue(SettingValueStr);
}

void USettingItem::SetOverallGraphycsValueStr(FString& SettingValueStr)
{
    SettingsTab->OverallGraphycsIndex = (SettingsTab->OverallGraphycsIndex + 1) % 5;
    SettingValueStr = GetGraphycValueStr(SettingsTab->OverallGraphycsIndex);
}

void USettingItem::SetViewDistanceValueStr(FString& SettingValueStr)
{
    SettingsTab->ViewDistanceIndex = (SettingsTab->ViewDistanceIndex + 1) % 5;
    SettingValueStr = GetGraphycValueStr(SettingsTab->ViewDistanceIndex);
}

void USettingItem::SetPostProcessingValueStr(FString& SettingValueStr)
{
    SettingsTab->PostProcessingIndex = (SettingsTab->PostProcessingIndex + 1) % 5;
    SettingValueStr = GetGraphycValueStr(SettingsTab->PostProcessingIndex);
}

void USettingItem::SetGlobalIlluminationValueStr(FString& SettingValueStr)
{
    SettingsTab->GlobalIlluminationIndex = (SettingsTab->GlobalIlluminationIndex + 1) % 5;
    SettingValueStr = GetGraphycValueStr(SettingsTab->GlobalIlluminationIndex);
}

void USettingItem::SetTexturesValueStr(FString& SettingValueStr)
{
    SettingsTab->TexturesIndex = (SettingsTab->TexturesIndex + 1) % 5;
    SettingValueStr = GetGraphycValueStr(SettingsTab->TexturesIndex);
}

void USettingItem::SetVisualEffectsValueStr(FString& SettingValueStr)
{
    SettingsTab->VisualEffectsIndex = (SettingsTab->VisualEffectsIndex + 1) % 5;
    SettingValueStr = GetGraphycValueStr(SettingsTab->VisualEffectsIndex);
}

void USettingItem::SetResolutionValueStr(FString& SettingValueStr)
{
    SettingsTab->ResolutionIndex = (SettingsTab->ResolutionIndex + 1) % 5;
    switch (SettingsTab->ResolutionIndex)
    {
        case 0:
            SettingsTab->Resolution.X = 1200;
            SettingsTab->Resolution.Y = 720;
            break;
        case 1:
            SettingsTab->Resolution.X = 1600;
            SettingsTab->Resolution.Y = 900;
            break;
        case 2:
            SettingsTab->Resolution.X = 1920;
            SettingsTab->Resolution.Y = 1080;
            break;
        case 3:
            SettingsTab->Resolution.X = 2560;
            SettingsTab->Resolution.Y = 1440;
            break;
        case 4:
            SettingsTab->Resolution.X = 3840;
            SettingsTab->Resolution.Y = 2160;
            break;
        default:
            SettingsTab->Resolution.X = 1920;
            SettingsTab->Resolution.Y = 1080;
            break;
    }
    SettingValueStr = GetResolutionValueStr(SettingsTab->Resolution);
}

void USettingItem::SetWindowModeValueStr(FString& SettingValueStr)
{
    int32 CurrentWindowMode = (int32)SettingsTab->WindowMode;
    CurrentWindowMode = (CurrentWindowMode + 1) % ((int32)EWindowMode::Type::Windowed + 1);
    SettingsTab->WindowMode = (EWindowMode::Type)CurrentWindowMode;
    SettingValueStr = GetWindowModeValueStr(SettingsTab->WindowMode);
}

void USettingItem::SetVsyncValueStr(FString& SettingValueStr)
{
    SettingsTab->bIsVsync = !SettingsTab->bIsVsync;
    SettingValueStr = GetVsyncValueStr(SettingsTab->bIsVsync);
}

void USettingItem::SettingButtonHovered()
{
    if (SettingTitle && SettingValue)
    {
        SettingTitle->SetColorAndOpacity(FColor::Green);
        SettingValue->SetColorAndOpacity(FColor::Green);
    }
}

void USettingItem::SettingButtonUnHovered()
{
    if (SettingTitle && SettingValue)
    {
        SettingTitle->SetColorAndOpacity(FColor::White);
        SettingValue->SetColorAndOpacity(FColor::White);
    }
}

FString USettingItem::GetSettingValue()
{
    FString SettingValueStr = FString();
    if (IsTabValid())
    {
        switch (Type)
        {
            // General Settings
            case ESettingType::ST_WindowMode: SettingValueStr = GetWindowModeValueStr(SettingsTab->WindowMode); break;
            case ESettingType::ST_Resolution: SettingValueStr = GetResolutionValueStr(SettingsTab->Resolution); break;
            case ESettingType::ST_Vsync: SettingValueStr = GetVsyncValueStr(SettingsTab->bIsVsync); break;

            // Graphycs Settings
            case ESettingType::ST_OverallGraphycs: SettingValueStr = GetGraphycValueStr(SettingsTab->OverallGraphycsIndex); break;
            case ESettingType::ST_ViewDistance: SettingValueStr = GetGraphycValueStr(SettingsTab->ViewDistanceIndex); break;
            case ESettingType::ST_PostProcessing: SettingValueStr = GetGraphycValueStr(SettingsTab->PostProcessingIndex); break;
            case ESettingType::ST_GlobalIllumination: SettingValueStr = GetGraphycValueStr(SettingsTab->GlobalIlluminationIndex); break;
            case ESettingType::ST_Textures: SettingValueStr = GetGraphycValueStr(SettingsTab->TexturesIndex); break;
            case ESettingType::ST_VisualEffects: SettingValueStr = GetGraphycValueStr(SettingsTab->VisualEffectsIndex); break;
            default: break;
        }
    }
    return SettingValueStr;
}

void USettingItem::SetSettingValue(const FString& Value)
{
    if (SettingValue)
    {
        SettingValue->SetText(FText::FromString(Value));
    }
}

FString USettingItem::GetWindowModeValueStr(const EWindowMode::Type& WindowModeType) const
{
    return UEnum::GetDisplayValueAsText(WindowModeType).ToString();
}

FString USettingItem::GetResolutionValueStr(const FIntPoint& Resolution) const
{
    return FString::Printf(TEXT("%dx%d"), Resolution.X, Resolution.Y);
}

FString USettingItem::GetVsyncValueStr(bool bIsVsync) const
{
    return bIsVsync ? FString("On") : FString("Off");
}

FString USettingItem::GetGraphycValueStr(int32 GraphycIndex) const
{
    switch (GraphycIndex)
    {
        case -1: return FString("Different"); break;
        case 0: return FString("Low"); break;
        case 1: return FString("Medium"); break;
        case 2: return FString("High"); break;
        case 3: return FString("Epic"); break;
        case 4: return FString("Ultra"); break;
        default: return FString("Medium"); break;
    }
}

bool USettingItem::IsTabValid()
{
    return SettingsTab != nullptr;
}
