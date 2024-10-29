// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/Button.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "BaseSettingsTab.h"
#include "SettingsMenu.h"

bool USettingsMenu::Initialize()
{
    bool result = Super::Initialize();
    if (GeneralButton && GraphycsButton && ApplyButton)
    {
        GeneralButton->OnClicked.AddDynamic(this, &ThisClass::GeneralButtonClicked);
        GraphycsButton->OnClicked.AddDynamic(this, &ThisClass::GraphycsButtonClicked);
        ApplyButton->OnClicked.AddDynamic(this, &ThisClass::ApplyButtonClicked);
    }

    // Click General button when create this widget
    if (GeneralButton && GeneralButton->OnClicked.IsBound())
    {
        GeneralButton->OnClicked.Broadcast();
    }
    return result;
}

void USettingsMenu::GeneralButtonClicked()
{
    if (!GeneralSettingsTabClass) return;
    if (GeneralButton)
    {
        if (GraphycsSettingsTab)
        {
            // Disable previous widget first
            GraphycsSettingsTab->RemoveFromParent();
            if (GraphycsButton)
            {
                GraphycsButton->SetIsEnabled(true);
            }
        }

        GeneralButton->SetIsEnabled(false);
        if (!GeneralSettingsTab)
        {
            GeneralSettingsTab = CreateWidget<UBaseSettingsTab>(this, GeneralSettingsTabClass);
        }
        if (GeneralSettingsTab)
        {
            GeneralSettingsTab->AddToViewport();
        }
    }
}

void USettingsMenu::GraphycsButtonClicked()
{
    if (!GraphycsSettingsTabClass) return;
    if (GraphycsButton)
    {
        // Disable previous widget first
        if (GeneralSettingsTab)
        {
            GeneralSettingsTab->RemoveFromParent();
            if (GeneralButton)
            {
                GeneralButton->SetIsEnabled(true);
            }
        }

        GraphycsButton->SetIsEnabled(false);
        if (!GraphycsSettingsTab)
        {
            GraphycsSettingsTab = CreateWidget<UBaseSettingsTab>(this, GraphycsSettingsTabClass);
        }
        if (GraphycsSettingsTab)
        {
            GraphycsSettingsTab->AddToViewport();
        }
    }
}

void USettingsMenu::ApplyButtonClicked()
{
    if (GEngine)
    {
        if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
        {
            if (GeneralSettingsTab)
            {
                UserSettings->SetFullscreenMode(GeneralSettingsTab->WindowMode);
                UserSettings->SetVSyncEnabled(GeneralSettingsTab->bIsVsync);
                UserSettings->SetScreenResolution(GeneralSettingsTab->Resolution);
            }
            if (GraphycsSettingsTab)
            {
                UserSettings->SetOverallScalabilityLevel(GraphycsSettingsTab->OverallGraphycsIndex);
                UserSettings->SetViewDistanceQuality(GraphycsSettingsTab->ViewDistanceIndex);
                UserSettings->SetPostProcessingQuality(GraphycsSettingsTab->PostProcessingIndex);
                UserSettings->SetGlobalIlluminationQuality(GraphycsSettingsTab->GlobalIlluminationIndex);
                UserSettings->SetTextureQuality(GraphycsSettingsTab->TexturesIndex);
                UserSettings->SetVisualEffectQuality(GraphycsSettingsTab->VisualEffectsIndex);

                // Hardcode shadows here for seeing fog in the game
                if (GraphycsSettingsTab->OverallGraphycsIndex < 2)
                {
                    UserSettings->SetShadowQuality(2);
                }
            }
            UserSettings->ApplySettings(true);
        }
    }
}
