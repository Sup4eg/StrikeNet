// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FHUDPackage
{
    GENERATED_BODY()
public:
    UTexture2D* CrosshairsCenter;
    UTexture2D* CrosshairsLeft;
    UTexture2D* CrosshairsRight;
    UTexture2D* CrosshairsTop;
    UTexture2D* CrosshairsBottom;
    float CrosshairSpread;
    FLinearColor CrosshairsColor;
};

UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    void DrawCrosshair(UTexture2D* Texture, const FVector2D& ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);
    bool IsHUDTexturesValid();

    FHUDPackage HUDPackage;

    UPROPERTY(EditAnywhere)
    float CrosshairSpreadMax = 16.f;

public:
    FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
