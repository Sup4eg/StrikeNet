// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Components/SlateWrapperTypes.h"
#include "BlasterPlayerController.generated.h"

class ABlasterHUD;
class UTexture;
class UCharacterOverlay;
class ABlasterGameMode;
class UInputMappingContext;
class UTextBlock;
class ABlasterCharacter;

UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void OnRep_Pawn() override;

    void SetHUDHealth(float Health, float MaxHealth);
    void SetHUDScore(float Score);
    void SetHUDDefeats(int32 Defeats);
    void SetHUDWeaponAmmo(int32 Ammo);
    void SetHUDCarriedAmmo(int32 Ammo);
    void SetHUDGrenadesAmount(int32 Grenades);
    void SetHUDWeaponIcon(UTexture2D* WeaponIcon);
    void HideHUDCharacterOverlay();
    void ShowHUDCharacterOverlay();
    void HideHUDWeaponInfo();
    void ShowHUDWeaponInfo();
    void HideHUDGrenadeInfo();
    void ShowHUDGrenadeInfo();
    void ShowHUDElimmed(const FName& KilledBy);
    void HideHUDElimmed();
    void SetHUDMatchCountdown(float CountdownTime);
    void SetHUDAnnouncementCountdown(float CountdownTime);

    virtual void Tick(float DeltaTime) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual float GetServerTime();           // Sync with server world clock
    virtual void ReceivedPlayer() override;  // Sync with server clock as soon as possible
    void OnMatchStateSet(FName State);

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

    void SetHUDTime();

    /**
     * Sync time between client and server
     */

    // Request the current server time, passing in the client's time when the request was sent
    UFUNCTION(Server, Reliable)
    void ServerRequeestServerTime(float TimeOfClientRequest);

    // Reports the current server time to the client in response to ServerRequestServerTime
    UFUNCTION(Client, Reliable)
    void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

    float ClientServerDelta = 0.f;  // difference between client and server time

    UPROPERTY(EditAnywhere, Category = "Time")
    float TimeSyncFrequency = 5.f;

    float TimeSyncRunningTime = 0.f;

    void CheckTimeSync(float DeltaTime);

private:
    bool IsHUDValid();
    bool IsCharacterOverlayValid();
    bool IsAnnouncementWidgetValid();
    void ShowHUDAnnouncement();
    void SetHUDCountdown(float CountdownTime, UTextBlock* TimeTextBlock);
    void SetLogicDependsOnMatchState(ABlasterCharacter* BlasterCharacter);

    float GetTimeLeft();
    uint32 GetSecondsLeft(float TimeLeft);

    UFUNCTION()
    void OnRep_MatchState();

    void HandleMatchState();
    void HandleMatchHasStarted();
    void HandleMatchCooldown();

    UFUNCTION(Server, Reliable)
    void ServerCheckMatchState();

    UFUNCTION(Client, Reliable)
    void ClientJoinMidgame(FName StateOfMatch,  //
        float Warmup,                           //
        float Match,                            //
        float StartingTime,                     //
        float TimeOfCooldown,                   //
        float TimeOfMatchLeftAlert);  //

    bool IsGameModeValid();

    UPROPERTY()
    ABlasterHUD* BlasterHUD;

    UPROPERTY()
    ABlasterGameMode* BlasterGameMode;

    /**
     * Input mapping context for cooldown
     */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputMappingContext* CooldownMappingContext;

    /** Time */
    float MatchTime = 0.f;
    float WarmupTime = 0.f;
    float MatchTimeLeftAlert = 0.f;
    float CooldownTime = 0.f;
    uint32 CountdownInt = 0;
    float LevelStartingTime = 0.f;

    UPROPERTY(ReplicatedUsing = OnRep_MatchState)
    FName MatchState;
};
