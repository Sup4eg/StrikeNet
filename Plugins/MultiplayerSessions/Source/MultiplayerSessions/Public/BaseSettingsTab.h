// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuTypes/SettingTypes.h"
#include "BaseSettingsTab.generated.h"

class UVerticalBox;
class USettingItem;

UCLASS()
class MULTIPLAYERSESSIONS_API UBaseSettingsTab : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * <GeneralSettings>
     */
    EWindowMode::Type WindowMode;
    int32 ResolutionIndex;
    FIntPoint Resolution;
    bool bIsVsync;
    /**
     * </GeneralSettings>
     */

    /**
     * <GraphycSettings>
     * */
    int32 OverallGraphycsIndex;
    int32 ViewDistanceIndex;
    int32 PostProcessingIndex;
    int32 GlobalIlluminationIndex;
    int32 TexturesIndex;
    int32 VisualEffectsIndex;
    /**
     * </GraphycSettings>
     **/

protected:
    virtual bool Initialize() override;

    UPROPERTY(EditDefaultsOnly)
    TMap<ESettingType, FSettingItemData> SettingItems;

    UPROPERTY(EditDefaultsOnly)
    TArray<ESettingType> SettingsItemOrder;

    UPROPERTY(meta = (BindWidget))
    UVerticalBox* SettingsBox;

    UPROPERTY(EditDefaultsOnly);
    TSubclassOf<USettingItem> SettingItemClass;
};
