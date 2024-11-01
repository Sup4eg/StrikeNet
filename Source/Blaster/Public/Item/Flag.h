// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarryItem.h"
#include "Flag.generated.h"

UCLASS()
class BLASTER_API AFlag : public ACarryItem
{
    GENERATED_BODY()

public:
    AFlag();


protected:
    virtual void BeginPlay() override;

    virtual void OnDropped() override;

private:
    UPROPERTY(VisibleAnywhere)
    USkeletalMeshComponent* CanvasMesh;
};
