// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UTexture2D;
class UCharacterOverlay;
class UAnnouncementWidget;
class UUserWidget;

USTRUCT(BlueprintType)
struct FHUDPackage
{
    GENERATED_BODY()
public:
    UPROPERTY()
    UTexture2D* CrosshairsCenter;

    UPROPERTY()
    UTexture2D* CrosshairsLeft;

    UPROPERTY()
    UTexture2D* CrosshairsRight;

    UPROPERTY()
    UTexture2D* CrosshairsTop;

    UPROPERTY()
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

    void AddAnnouncementWidget();

    UPROPERTY(EditAnywhere, Category = "Player Stats")
    TSubclassOf<UUserWidget> CharacterOverlayClass;

    UPROPERTY(EditAnywhere, Category = "Announcements")
    TSubclassOf<UUserWidget> AnnouncementWidgetClass;

    UPROPERTY()
    UCharacterOverlay* CharacterOverlay;

    UPROPERTY()
    UAnnouncementWidget* AnnouncementWidget;

protected:
    virtual void BeginPlay() override;

private:
    void AddCharacterOverlay();

    void DrawCrosshair(UTexture2D* Texture, const FVector2D& ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);
    bool IsHUDTexturesValid();

    FHUDPackage HUDPackage;

    UPROPERTY(EditAnywhere)
    float CrosshairSpreadMax = 16.f;

    bool bIsDrawCrosshair = false;

public:
    FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
    FORCEINLINE void SetIsDrawCrosshair(bool bDraw) { bIsDrawCrosshair = bDraw; }
};
