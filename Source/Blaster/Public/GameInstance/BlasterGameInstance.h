// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BlasterGameInstance.generated.h"

/**
 *
 */
UCLASS()
class BLASTER_API UBlasterGameInstance : public UGameInstance
{
    GENERATED_BODY()

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    bool bLoadingScreen = true;
};
