#pragma once

#include "SettingTypes.generated.h"

UENUM(BlueprintType)
enum class ESettingType : uint8
{
    /**
     * <GeneralSettings>
     */
    ST_WindowMode UMETA(DisplayName = "Window Mode"),
    ST_Resolution UMETA(DisplayName = "Resolution"),
    ST_Vsync UMETA(DispalyName = "Vsync"),
    /**
     * </GeneralSettings
     */

    /**
     * <GraphycSettings>
     */
    ST_OverallGraphycs UMETA(DisplayName = "Overall Graphycs"),
    ST_ViewDistance UMETA(DisplayName = "View Distance"),
    ST_PostProcessing UMETA(DisplayName = "Post Processing"),
    ST_GlobalIllumination UMETA(DisplayName = "Global Illumination"),
    ST_Textures UMETA(DisplayName = "Textures"),
    ST_VisualEffects UMETA(DisplayName = "Visual Effects"),
    /**
     * </GraphycSettings>
     */

    ST_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FSettingItemData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditDefaultsOnly)
    FString SettingTitle = FString();
};
