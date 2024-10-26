// Fill out your copyright notice in the Description page of Project Settings.

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "AnnouncementWidget.h"
#include "ElimAnnouncementWidget.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Components/HorizontalBox.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "BlasterHUD.h"

void ABlasterHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!bIsDrawCrosshair || !GEngine || !GEngine->GameViewport) return;
    FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);
    const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
    float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;
    DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, FVector2D(0.f, 0.f), HUDPackage.CrosshairsColor);
    DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, FVector2D(-SpreadScaled, 0.f), HUDPackage.CrosshairsColor);
    DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, FVector2D(SpreadScaled, 0.f), HUDPackage.CrosshairsColor);
    DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, FVector2D(0.f, -SpreadScaled), HUDPackage.CrosshairsColor);
    DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, FVector2D(0.f, SpreadScaled), HUDPackage.CrosshairsColor);
}

void ABlasterHUD::BeginPlay()
{
    Super::BeginPlay();
    AddCharacterOverlay();
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, const FVector2D& ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor)
{
    if (!Texture) return;
    const float TextureWidth = Texture->GetSizeX();
    const float TextureHeight = Texture->GetSizeY();
    const FVector2D TextureDrawPoint(                        //
        ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,  //
        ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y  //
    );
    DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHeight, 0.f, 0.f, 1.f, 1.f, CrosshairColor);
}

void ABlasterHUD::AddCharacterOverlay()
{
    APlayerController* PlayerController = GetOwningPlayerController();
    if (!PlayerController || !CharacterOverlayClass || CharacterOverlay) return;
    CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
    if (CharacterOverlay)
    {
        CharacterOverlay->AddToViewport();
        CharacterOverlay->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ABlasterHUD::AddAnnouncementWidget()
{
    APlayerController* PlayerController = GetOwningPlayerController();
    if (!PlayerController || !AnnouncementWidgetClass || AnnouncementWidget) return;
    AnnouncementWidget = CreateWidget<UAnnouncementWidget>(PlayerController, AnnouncementWidgetClass);
    if (AnnouncementWidget)
    {
        AnnouncementWidget->AddToViewport();
    }
}

void ABlasterHUD::AddElimAnnouncementWidget(FString Attacker, FString Victim)
{
    APlayerController* PlayerController = GetOwningPlayerController();
    if (!PlayerController || !ElimAnnouncementWidgetClass) return;
    UElimAnnouncementWidget* ElimAnnouncementWidget = CreateWidget<UElimAnnouncementWidget>(PlayerController, ElimAnnouncementWidgetClass);
    if (ElimAnnouncementWidget)
    {
        ElimAnnouncementWidget->SetElimAnnouncementText(Attacker, Victim);
        ElimAnnouncementWidget->AddToViewport();

        for (UElimAnnouncementWidget*& Msg : ElimMessages)
        {
            if (Msg && Msg->AnnouncementBox)
            {
                if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg->AnnouncementBox))
                {
                    FVector2D Position = CanvasSlot->GetPosition();
                    FVector2D NewPosition(Position.X, Position.Y - CanvasSlot->GetSize().Y);
                    CanvasSlot->SetPosition(NewPosition);
                }
            }
        }

        ElimMessages.Add(ElimAnnouncementWidget);

        FTimerHandle ElimMsgTimer;
        FTimerDelegate ElimMsgDelegate;
        ElimMsgDelegate.BindUFunction(this, FName("ElimAnnouncementTimerFinished"), ElimAnnouncementWidget);

        GetWorldTimerManager().SetTimer(  //
            ElimMsgTimer,                 //
            ElimMsgDelegate,              //
            ElimAnnouncementTime,         //
            false);
    }
}

void ABlasterHUD::ElimAnnouncementTimerFinished(UElimAnnouncementWidget* MsgToRemove)
{
    if (MsgToRemove)
    {
        ElimMessages.Remove(MsgToRemove);
        MsgToRemove->RemoveFromParent();
    }
}
