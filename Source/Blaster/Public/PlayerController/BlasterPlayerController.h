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
class UInputAction;
class UTextBlock;
class ABlasterCharacter;
class UPauseWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);
DECLARE_MULTICAST_DELEGATE(FPlayerCharacterBeginPlay);

UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void OnRep_Pawn() override;

    void SetHUDHealth(float Health, float MaxHealth);
    void SetHUDShield(float Shield, float MaxShield);
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
    void ShowPauseWidget(ABlasterCharacter* PlayerCharacter);

    virtual void Tick(float DeltaTime) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual float GetServerTime();           // Sync with server world clock
    virtual void ReceivedPlayer() override;  // Sync with server clock as soon as possible
    void OnMatchStateSet(FName State);

    float SingleTripTime = 0.f;

    FHighPingDelegate HighPingDelegate;

    FPlayerCharacterBeginPlay OnPlayerCharacterBeginPlay;

    bool bPauseWidgetOpen = false;

protected:
    virtual void BeginPlay() override;

    virtual void SetupInputComponent() override;

    virtual void OnPossess(APawn* InPawn) override;

    void SetHUDTime();

    void SetUpInputMappingContext(UInputMappingContext* MappingContext);

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

    void HighPingWarning();
    void StopHighPingWarning();
    void CheckPing(float DeltaTime);

    /** Callbacks for input */
    void ShowPauseWidget();

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* QuitAction;

private:
    bool IsHUDValid();
    bool IsCharacterOverlayValid();
    bool IsAnnouncementWidgetValid();
    void ShowHUDAnnouncement();
    void SetHUDCountdown(float CountdownTime, UTextBlock* TimeTextBlock);
    void SetLogicDependsOnMatchState();
    bool IsBlasterCharacterValid();

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
        float TimeOfMatchLeftAlert);            //

    bool IsGameModeValid();

    UFUNCTION(Server, Reliable)
    void ServerReportPingStatus(bool bHighPing);

    UPROPERTY()
    ABlasterHUD* BlasterHUD;

    UPROPERTY()
    ABlasterGameMode* BlasterGameMode;

    /**
     * Input mapping context for cooldown
     */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputMappingContext* CooldownMappingContext;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputMappingContext* InGameMappingContext;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputMappingContext* PauseMappingContext;

    /** Time */
    float MatchTime = 0.f;
    float WarmupTime = 0.f;
    float MatchTimeLeftAlert = 0.f;
    float CooldownTime = 0.f;
    uint32 CountdownInt = 0;
    float LevelStartingTime = 0.f;

    UPROPERTY(ReplicatedUsing = OnRep_MatchState)
    FName MatchState;

    float HighPingRunningTime = 20.f;

    UPROPERTY(EditAnywhere)
    float HightPingDuration = 5.f;

    float PingAnimationRunningTime = 0.f;

    UPROPERTY(EditAnywhere)
    float CheckPingFrequency = 20.f;

    UPROPERTY(EditDefaultsOnly)
    float HighPingThreshold = 50.f;

    /**
     * Return to main menu
     */

    UPROPERTY(EditDefaultsOnly, Category = HUD)
    TSubclassOf<UUserWidget> PauseWidgetClass;

    UPROPERTY()
    UPauseWidget* PauseWidget;

    UPROPERTY()
    UInputMappingContext* LastMappingContext;

    UPROPERTY()
    TWeakObjectPtr<ABlasterCharacter> BlasterCharacter;

public:
    FORCEINLINE UInputMappingContext* GetLastMappingContext() const { return LastMappingContext; };
};
