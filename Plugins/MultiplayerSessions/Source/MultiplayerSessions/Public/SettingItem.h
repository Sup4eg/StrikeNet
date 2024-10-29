// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuTypes/SettingTypes.h"
#include "SettingItem.generated.h"

class UButton;
class UTextBlock;
class UBaseSettingsTab;

UCLASS()
class MULTIPLAYERSESSIONS_API USettingItem : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetData(FString NewSettingTitle, ESettingType NewSettingType, UBaseSettingsTab* TabToSet);

protected:
    virtual bool Initialize() override;

private:
    bool IsTabValid();

    /**<GeneralSettings> */
    FString GetSettingValue();
    FString GetResolutionValueStr(const FIntPoint& Resolution) const;
    FString GetVsyncValueStr(bool bIsVsync) const;
    FString GetWindowModeValueStr(const EWindowMode::Type& WindowModeType) const;
    void SetSettingValue(const FString& Value);
    void SetWindowModeValueStr(FString& SettingValueStr);
    void SetResolutionValueStr(FString& SettingValueStr);
    void SetVsyncValueStr(FString& SettingValueStr);
    /**</GeneralSettings> */

    /**<GraphycSettings> */
    FString GetGraphycValueStr(int32 GraphycIndex) const;
    void SetOverallGraphycsValueStr(FString& SettingValueStr);
    void SetViewDistanceValueStr(FString& SettingValueStr);
    void SetPostProcessingValueStr(FString& SettingValueStr);
    void SetGlobalIlluminationValueStr(FString& SettingValueStr);
    void SetTexturesValueStr(FString& SettingValueStr);
    void SetVisualEffectsValueStr(FString& SettingValueStr);
    /**</GraphycSettings> */

    UFUNCTION()
    void SettingButtonClicked();

    UFUNCTION()
    void SettingButtonHovered();

    UFUNCTION()
    void SettingButtonUnHovered();

    UPROPERTY(meta = (BindWidget))
    UButton* SettingButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SettingTitle;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SettingValue;

    ESettingType Type = ESettingType::ST_MAX;

    UPROPERTY()
    UBaseSettingsTab* SettingsTab;
};
