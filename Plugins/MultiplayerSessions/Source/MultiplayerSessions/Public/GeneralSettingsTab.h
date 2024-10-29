// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseSettingsTab.h"
#include "GeneralSettingsTab.generated.h"

UCLASS()
class MULTIPLAYERSESSIONS_API UGeneralSettingsTab : public UBaseSettingsTab
{
    GENERATED_BODY()

protected:
    virtual bool Initialize() override;
};
