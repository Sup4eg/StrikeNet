// Fill out your copyright notice in the Description page of Project Settings.

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "GraphycSettingsTab.h"

bool UGraphycSettingsTab::Initialize()
{
    if (GEngine)
    {
        if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
        {
            UserSettings->LoadSettings();
            OverallGraphycsIndex = UserSettings->GetOverallScalabilityLevel();
            ViewDistanceIndex = UserSettings->GetViewDistanceQuality();
            PostProcessingIndex = UserSettings->GetPostProcessingQuality();
            GlobalIlluminationIndex = UserSettings->GetGlobalIlluminationQuality();
            TexturesIndex = UserSettings->GetTextureQuality();
            VisualEffectsIndex = UserSettings->GetVisualEffectQuality();
        }
    }

    return Super::Initialize();
}
