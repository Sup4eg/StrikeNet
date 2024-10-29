// Fill out your copyright notice in the Description page of Project Settings.

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "GeneralSettingsTab.h"

bool UGeneralSettingsTab::Initialize()
{
    if (GEngine)
    {
        if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
        {
            UserSettings->LoadSettings();
            WindowMode = UserSettings->GetFullscreenMode();
            bIsVsync = UserSettings->IsVSyncEnabled();
            Resolution = UserSettings->GetScreenResolution();
            switch (Resolution.X)
            {
                case 1280: ResolutionIndex = 0; break;
                case 1600: ResolutionIndex = 1; break;
                case 1920: ResolutionIndex = 2; break;
                case 2560: ResolutionIndex = 3; break;
                case 3840: ResolutionIndex = 4; break;
                default: ResolutionIndex = 2; break;
            }
        }
    }

    return Super::Initialize();
}
