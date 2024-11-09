#include "BlasterPlayerController.h"
// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterHUD.h"
#include "CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.h"
#include "ElimmedWidget.h"
#include "AnnouncementWidget.h"
#include "BlasterCharacter.h"
#include "BlasterUtils.h"
#include "Engine/World.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "BlasterGameMode.h"
#include "CombatComponent.h"
#include "BlasterGameState.h"
#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "PauseWidget.h"
#include "Announcement.h"
#include "BlasterPlayerController.h"

void ABlasterPlayerController::BeginPlay()
{
    Super::BeginPlay();
    BlasterHUD = Cast<ABlasterHUD>(GetHUD());
    ServerCheckMatchState();
    Tags.Add("BlasterPlayerController");
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    SetHUDTime();
    CheckTimeSync(DeltaTime);
    CheckPing(DeltaTime);
}

void ABlasterPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (!InputComponent) return;

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        EnhancedInputComponent->BindAction(QuitAction, ETriggerEvent::Completed, this, &ThisClass::ShowPauseWidget);
    }
}

void ABlasterPlayerController::CheckPing(float DeltaTime)
{
    HighPingRunningTime += DeltaTime;
    if (HighPingRunningTime > CheckPingFrequency)
    {
        if (!PlayerState)
        {
            PlayerState = GetPlayerState<APlayerState>();
        }
        if (PlayerState)
        {
            if (PlayerState->GetPingInMilliseconds() > HighPingThreshold)
            {
                HighPingWarning();
                PingAnimationRunningTime = 0.f;
                ServerReportPingStatus(true);
            }
            else
            {
                ServerReportPingStatus(false);
            }
        }
        HighPingRunningTime = 0.f;
    }

    bool bHighPingAnimationPlaying = IsCharacterOverlayValid() &&                        //
                                     BlasterHUD->CharacterOverlay->HighPingAnimation &&  //                                          //
                                     BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation);  //

    if (bHighPingAnimationPlaying)
    {
        PingAnimationRunningTime += DeltaTime;
        if (PingAnimationRunningTime > HightPingDuration)
        {
            StopHighPingWarning();
        }
    }
}

void ABlasterPlayerController::ShowPauseWidget()
{
    BlasterCharacter = Cast<ABlasterCharacter>(GetCharacter());
    ShowPauseWidget(BlasterCharacter.Get());
}

void ABlasterPlayerController::ShowPauseWidget(ABlasterCharacter* PlayerCharacter)
{
    if (!PauseWidgetClass) return;
    if (!PauseWidget)
    {
        PauseWidget = CreateWidget<UPauseWidget>(this, PauseWidgetClass);
    }
    if (PauseWidget)
    {
        bPauseWidgetOpen = !bPauseWidgetOpen;
        if (bPauseWidgetOpen)
        {
            PauseWidget->MenuSetup();
            SetUpInputMappingContext(PauseMappingContext);
            LastMappingContext = PauseMappingContext;
            if (PlayerCharacter != nullptr)
            {
                PlayerCharacter->SetIsGameplayDisabled(true);
                if (PlayerCharacter->GetCombatComponent())
                {
                    if (PlayerCharacter->IsAiming())
                    {
                        PlayerCharacter->GetCombatComponent()->SetAiming(false);
                    }
                    PlayerCharacter->GetCombatComponent()->FireButtonPressed(false);
                }
            }
        }
        else
        {
            PauseWidget->MenuTearDown();
            if (PlayerCharacter != nullptr)
            {
                PlayerCharacter->SetIsGameplayDisabled(PlayerCharacter->GetIsElimmed());
                SetUpInputMappingContext(PlayerCharacter->GetLastMappingContext());
            }
            else
            {
                PlayerCharacter->SetIsGameplayDisabled(false);
                SetUpInputMappingContext(InGameMappingContext);
            }
            LastMappingContext = nullptr;
        }
    }
}

void ABlasterPlayerController::SetUpInputMappingContext(UInputMappingContext* MappingContext)
{
    if (!MappingContext) return;
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->ClearAllMappings();
        Subsystem->AddMappingContext(MappingContext, 0);
    }
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABlasterPlayerController, MatchState);
    DOREPLIFETIME(ABlasterPlayerController, bShowTeamScores);
}

void ABlasterPlayerController::OnRep_Pawn()
{
    Super::OnRep_Pawn();
    HideHUDElimmed();
    if (IsBlasterCharacterValid())
    {
        SetLogicDependsOnMatchState();
    }
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (IsBlasterCharacterValid())
    {
        SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
        SetHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
        BlasterCharacter->UpdateHUDAmmo();
        HideHUDElimmed();
        SetLogicDependsOnMatchState();
    }
}

void ABlasterPlayerController::SetLogicDependsOnMatchState()
{
    if (MatchState == MatchState::Cooldown)
    {
        BlasterCharacter->SetIsGameplayDisabled(true);
        BlasterCharacter->SetUpInputMappingContext(CooldownMappingContext);
    }
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                //
                     BlasterHUD->CharacterOverlay->HealthBar &&  //
                     BlasterHUD->CharacterOverlay->HealthText;
    if (bHUDValid)
    {
        const float HealthPercent = Health / MaxHealth;
        const FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
        BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
        BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
    }
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                //
                     BlasterHUD->CharacterOverlay->ShieldBar &&  //
                     BlasterHUD->CharacterOverlay->ShieldText;
    if (bHUDValid)
    {
        const float ShieldPercent = Shield / MaxShield;
        const FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
        BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
        BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
    }
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
    bool bHUDValid = IsCharacterOverlayValid() && BlasterHUD->CharacterOverlay->ScoreAmount;
    if (bHUDValid)
    {
        FString ScoreText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Score));
        BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
    }
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
    bool bHUDValid = IsCharacterOverlayValid() && BlasterHUD->CharacterOverlay->DefeatsAmount;
    if (bHUDValid)
    {
        FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
        BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
    }
}

void ABlasterPlayerController::ShowHUDElimmed(const FName& KilledBy)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                              //
                     BlasterHUD->CharacterOverlay->ElimmedWidget &&            //
                     BlasterHUD->CharacterOverlay->ElimmedWidget->KillText &&  //
                     !KilledBy.IsEqual("") &&                                  //
                     !KilledBy.IsNone();

    if (bHUDValid)
    {
        FString KillTextString = FString("");
        ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();

        // Suicide
        if (BlasterPlayerState->GetPlayerName().Equals(KilledBy.ToString()))
        {
            KillTextString = "Self destructed";
        }
        else
        {
            KillTextString = FString::Printf(TEXT("Killed by %s"), *KilledBy.ToString());
        }

        if (KillTextString != FString(""))
        {
            BlasterHUD->CharacterOverlay->ElimmedWidget->KillText->SetText(FText::FromString(KillTextString));
            BlasterHUD->CharacterOverlay->ElimmedWidget->SetVisibility(ESlateVisibility::Visible);
        }
    }
}

void ABlasterPlayerController::HideHUDElimmed()
{
    bool bHUDValid = IsCharacterOverlayValid() &&                    //
                     BlasterHUD->CharacterOverlay->ElimmedWidget &&  //
                     BlasterHUD->CharacterOverlay->ElimmedWidget->KillText;

    if (bHUDValid)
    {
        BlasterHUD->CharacterOverlay->ElimmedWidget->KillText->SetText(FText());
        BlasterHUD->CharacterOverlay->ElimmedWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                     //
                     BlasterHUD->CharacterOverlay->WeaponAmmoAmount;  //

    if (bHUDValid)
    {
        FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
        BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
    }
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                      //
                     BlasterHUD->CharacterOverlay->CarriedAmmoAmount;  //

    if (bHUDValid)
    {
        FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
        BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
    }
}

void ABlasterPlayerController::SetHUDGrenadesAmount(int32 Grenades)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                   //
                     BlasterHUD->CharacterOverlay->GrenadesAmount;  //

    if (bHUDValid)
    {
        FString GrenadesAmountText = FString::Printf(TEXT("%d"), Grenades);
        BlasterHUD->CharacterOverlay->GrenadesAmount->SetText(FText::FromString(GrenadesAmountText));
    }
}

void ABlasterPlayerController::SetHUDWeaponIcon(UTexture2D* WeaponIcon)
{

    bool bHUDValid = IsCharacterOverlayValid() &&              //
                     BlasterHUD->CharacterOverlay->WeaponImg;  //
    if (bHUDValid && WeaponIcon)
    {
        BlasterHUD->CharacterOverlay->WeaponImg->SetBrushFromTexture(WeaponIcon);
    }
}

void ABlasterPlayerController::HideHUDCharacterOverlay()
{
    if (IsCharacterOverlayValid())
    {
        BlasterHUD->CharacterOverlay->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ABlasterPlayerController::ShowHUDCharacterOverlay()
{
    if (IsCharacterOverlayValid())
    {
        BlasterHUD->CharacterOverlay->SetVisibility(ESlateVisibility::Visible);
    }
}

void ABlasterPlayerController::HideHUDWeaponInfo()
{
    bool bHUDValid = IsCharacterOverlayValid() &&               //
                     BlasterHUD->CharacterOverlay->WeaponInfo;  //
    if (bHUDValid)
    {
        BlasterHUD->CharacterOverlay->WeaponInfo->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ABlasterPlayerController::ShowHUDWeaponInfo()
{
    bool bHUDValid = IsCharacterOverlayValid() &&               //
                     BlasterHUD->CharacterOverlay->WeaponInfo;  //
    if (bHUDValid)
    {
        BlasterHUD->CharacterOverlay->WeaponInfo->SetVisibility(ESlateVisibility::Visible);
    }
}

void ABlasterPlayerController::HideHUDGrenadeInfo()
{
    bool bHUDValid = IsCharacterOverlayValid() &&                //
                     BlasterHUD->CharacterOverlay->GrenadeInfo;  //
    if (bHUDValid)
    {
        BlasterHUD->CharacterOverlay->GrenadeInfo->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ABlasterPlayerController::ShowHUDGrenadeInfo()
{
    bool bHUDValid = IsCharacterOverlayValid() &&                //
                     BlasterHUD->CharacterOverlay->GrenadeInfo;  //
    if (bHUDValid)
    {
        BlasterHUD->CharacterOverlay->GrenadeInfo->SetVisibility(ESlateVisibility::Visible);
    }
}

void ABlasterPlayerController::HideTeamScores()
{
    bool bHUDValid = IsCharacterOverlayValid() &&                  //
                     BlasterHUD->CharacterOverlay->TeamScoreInfo;  //
    if (bHUDValid)
    {
        BlasterHUD->CharacterOverlay->TeamScoreInfo->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ABlasterPlayerController::InitTeamScores()
{
    bool bHUDValid = IsCharacterOverlayValid() &&                    //
                     BlasterHUD->CharacterOverlay->TeamScoreInfo &&  //
                     BlasterHUD->CharacterOverlay->RedTeamScore &&   //
                     BlasterHUD->CharacterOverlay->BlueTeamScore;    //

    if (bHUDValid)
    {
        BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString("0"));
        BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString("0"));
        BlasterHUD->CharacterOverlay->TeamScoreInfo->SetVisibility(ESlateVisibility::Visible);
    }
}

void ABlasterPlayerController::SetHUDRedTeamScore(int32 RedTeamScore)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                 //
                     BlasterHUD->CharacterOverlay->RedTeamScore;  //

    if (bHUDValid)
    {
        FString RedTeamScoreStr = FString::Printf(TEXT("%d"), RedTeamScore);
        BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(RedTeamScoreStr));
    }
}

void ABlasterPlayerController::SetHUDBlueTeamScore(int32 BlueTeamScore)
{
    bool bHUDValid = IsCharacterOverlayValid() &&                  //
                     BlasterHUD->CharacterOverlay->BlueTeamScore;  //

    if (bHUDValid)
    {
        FString BlueTeamScoreStr = FString::Printf(TEXT("%d"), BlueTeamScore);
        BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(BlueTeamScoreStr));
    }
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{

    bool bHUDValid = IsCharacterOverlayValid() &&                       //
                     BlasterHUD->CharacterOverlay->MatchCountdownText;  //
    if (bHUDValid)
    {
        SetHUDCountdown(CountdownTime, BlasterHUD->CharacterOverlay->MatchCountdownText);
        BlasterHUD->CharacterOverlay->MatchCountdownText->SetColorAndOpacity(FColor::White);
        if (CountdownTime <= MatchTimeLeftAlert)
        {
            BlasterHUD->CharacterOverlay->MatchCountdownText->SetColorAndOpacity(FColor::Red);
            BlasterHUD->CharacterOverlay->PlayAnimation(BlasterHUD->CharacterOverlay->CountdownAnimation);
        }
    }
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
    bool bHUDValid = IsAnnouncementWidgetValid() && BlasterHUD->AnnouncementWidget->WarmupTime;
    if (bHUDValid)
    {

        SetHUDCountdown(CountdownTime, BlasterHUD->AnnouncementWidget->WarmupTime);
    }
}

void ABlasterPlayerController::HighPingWarning()
{
    bool bHUDValid = IsCharacterOverlayValid() &&                  //
                     BlasterHUD->CharacterOverlay->HighPingImg &&  //
                     BlasterHUD->CharacterOverlay->HighPingAnimation;
    if (bHUDValid)
    {
        BlasterHUD->CharacterOverlay->HighPingImg->SetOpacity(1.f);
        BlasterHUD->CharacterOverlay->PlayAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation, 0.f, 5);
    }
}

void ABlasterPlayerController::StopHighPingWarning()
{
    bool bHUDValid = IsCharacterOverlayValid() &&                  //
                     BlasterHUD->CharacterOverlay->HighPingImg &&  //
                     BlasterHUD->CharacterOverlay->HighPingAnimation;
    if (bHUDValid)
    {
        BlasterHUD->CharacterOverlay->HighPingImg->SetOpacity(0.f);
        if (BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation))
        {
            BlasterHUD->CharacterOverlay->StopAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation);
        }
    }
}

void ABlasterPlayerController::SetHUDCountdown(float CountdownTime, UTextBlock* TimeTextBlock)
{
    if (CountdownTime < 0.f)
    {
        TimeTextBlock->SetText(FText::FromString(FString("00:00")));
        return;
    }

    int32 Minutes = FMath::FloorToInt(CountdownTime / 60);
    int32 Seconds = CountdownTime - Minutes * 60;

    FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
    TimeTextBlock->SetText(FText::FromString(CountdownText));
}

void ABlasterPlayerController::SetHUDTime()
{
    if (!GetWorld()) return;

    float TimeLeft = GetTimeLeft();
    uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

    if (CountdownInt != SecondsLeft)
    {
        if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
        {
            SetHUDAnnouncementCountdown(TimeLeft);
        }
        if (MatchState == MatchState::InProgress)
        {
            SetHUDMatchCountdown(TimeLeft);
        }
    }
    CountdownInt = SecondsLeft;
}

float ABlasterPlayerController::GetTimeLeft()
{
    float TimeLeft = 0.f;

    if (HasAuthority() && IsGameModeValid())
    {
        TimeLeft = BlasterGameMode->GetCountdownTime();
    }
    else
    {
        if (MatchState == MatchState::WaitingToStart)
        {
            TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
        }
        else if (MatchState == MatchState::InProgress)
        {
            TimeLeft = MatchTime + WarmupTime - GetServerTime() + LevelStartingTime;
        }
        else if (MatchState == MatchState::Cooldown)
        {
            TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
        }
    }

    return TimeLeft;
}

uint32 ABlasterPlayerController::GetSecondsLeft(float TimeLeft)
{
    uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

    if (HasAuthority() && IsGameModeValid())
    {
        SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + BlasterGameMode->LevelStartingTime);
    }
    return SecondsLeft;
}

void ABlasterPlayerController::ServerRequeestServerTime_Implementation(float TimeOfClientRequest)
{
    if (!GetWorld()) return;
    float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
    ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
    if (!GetWorld()) return;
    float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
    SingleTripTime = RoundTripTime / 2;
    float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
    ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
    TimeSyncRunningTime += DeltaTime;
    if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
    {
        ServerRequeestServerTime(GetWorld()->GetTimeSeconds());
        TimeSyncRunningTime = 0.f;
    }
}

float ABlasterPlayerController::GetServerTime()
{
    if (!GetWorld()) return 0.f;
    if (HasAuthority())
    {
        return GetWorld()->GetTimeSeconds();
    }
    else
    {
        return GetWorld()->GetTimeSeconds() + ClientServerDelta;
    }
}

void ABlasterPlayerController::ReceivedPlayer()
{
    Super::ReceivedPlayer();
    if (IsLocalController())
    {
        ServerRequeestServerTime(GetWorld()->GetTimeSeconds());
    }
}

void ABlasterPlayerController::OnMatchStateSet(FName State, bool bTeamsMatch)
{
    MatchState = State;
    HandleMatchState(bTeamsMatch);
}

void ABlasterPlayerController::OnRep_MatchState()
{
    HandleMatchState();
}

void ABlasterPlayerController::HandleMatchState(bool bTeamsMatch)
{
    if (MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted(bTeamsMatch);
    }
    else if (MatchState == MatchState::Cooldown)
    {
        HandleMatchCooldown();
    }
}

void ABlasterPlayerController::HandleMatchHasStarted(bool bTeamsMatch)
{
    if (HasAuthority())
    {
        bShowTeamScores = bTeamsMatch;
        if (bTeamsMatch)
        {
            InitTeamScores();
        }
        else
        {
            HideTeamScores();
        }
    }

    if (IsCharacterOverlayValid())
    {
        if (GetCharacter()->ActorHasTag("BlasterCharacter"))
        {
            if (IsBlasterCharacterValid())
            {
                BlasterCharacter->UpdateHUDAmmo();
            }
        }
        BlasterHUD->CharacterOverlay->SetVisibility(ESlateVisibility::Visible);
    }
    if (IsAnnouncementWidgetValid())
    {
        BlasterHUD->AnnouncementWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ABlasterPlayerController::HandleMatchCooldown()
{
    if (IsCharacterOverlayValid())
    {
        BlasterHUD->CharacterOverlay->RemoveFromParent();
    }
    if (IsHUDValid())
    {
        BlasterHUD->AddAnnouncementWidget();
        if (BlasterHUD->AnnouncementWidget && BlasterHUD->AnnouncementWidget->AnnouncementText && BlasterHUD->AnnouncementWidget->InfoText)
        {
            ShowHUDAnnouncement();
        }
    }
    if (IsBlasterCharacterValid())
    {
        BlasterCharacter->SetIsGameplayDisabled(true);
        BlasterCharacter->SetUpInputMappingContext(CooldownMappingContext);
        if (BlasterCharacter->GetCombatComponent())
        {
            if (BlasterCharacter->IsAiming())
            {
                BlasterCharacter->GetCombatComponent()->SetAiming(false);
            }
            BlasterCharacter->GetCombatComponent()->FireButtonPressed(false);
        }
    }
}

void ABlasterPlayerController::ShowHUDAnnouncement()
{

    FString AnnouncementText = Announcement::NewMatchStartsIn;
    BlasterHUD->AnnouncementWidget->AnnouncementText->SetText(FText::FromString(AnnouncementText));

    if (ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)))
    {
        TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
        FString InfoTextString = bShowTeamScores ? GetTeamsInfoText(BlasterGameState) : GetInfoText(TopPlayers);
        if (InfoTextString != FString(""))
        {
            BlasterHUD->AnnouncementWidget->InfoText->SetText(FText::FromString(InfoTextString));
        }
    }
    BlasterHUD->AnnouncementWidget->SetVisibility(ESlateVisibility::Visible);
}

FString ABlasterPlayerController::GetInfoText(const TArray<ABlasterPlayerState*>& Players)
{
    ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
    if (!BlasterPlayerState) return FString();

    FString InfoTextString;
    if (Players.IsEmpty())
    {
        InfoTextString = Announcement::ThereIsNoWinner;
    }
    else if (Players.Num() == 1 && Players[0] == BlasterPlayerState)
    {
        InfoTextString = Announcement::YouAreTheWinner;
    }
    else if (Players.Num() == 1)
    {
        InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *Players[0]->GetPlayerName());
    }
    else if (Players.Num() > 1)
    {
        InfoTextString = Announcement::PlayersTiedForTheWin;
        InfoTextString.Append(FString("\n"));
        for (auto TiedPlayer : Players)
        {
            InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
        }
    }
    return InfoTextString;
}

FString ABlasterPlayerController::GetTeamsInfoText(ABlasterGameState* BlasterGameState)
{
    if (!BlasterGameState) return FString();
    FString InfoTextString;

    const int32 RedTeamScore = BlasterGameState->RedTeamScore;
    const int32 BlueTeamScore = BlasterGameState->BlueTeamScore;

    if (RedTeamScore == 0 && BlueTeamScore == 0)
    {
        InfoTextString = Announcement::ThereIsNoWinner;
    }
    else if (RedTeamScore == BlueTeamScore)
    {
        InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::TeamsTiedForTheWin);
        InfoTextString.Append(Announcement::RedTeam);
        InfoTextString.Append(TEXT("\n"));
        InfoTextString.Append(Announcement::BlueTeam);
        InfoTextString.Append(TEXT("\n"));
    }
    else if (RedTeamScore > BlueTeamScore)
    {
        InfoTextString = Announcement::RedTeamWins;
        InfoTextString.Append(TEXT("\n"));
        InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
        InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
    }
    else if (BlueTeamScore > RedTeamScore)
    {
        InfoTextString = Announcement::BlueTeamWins;
        InfoTextString.Append(TEXT("\n"));
        InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
        InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
    }
    return InfoTextString;
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
    ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GameMode) return;

    WarmupTime = GameMode->WarmupTime;
    MatchTimeLeftAlert = GameMode->MatchTimeLeftAlert;
    MatchTime = GameMode->MatchTime;
    LevelStartingTime = GameMode->LevelStartingTime;
    MatchState = GameMode->GetMatchState();
    CooldownTime = GameMode->CooldownTime;
    ClientJoinMidgame(MatchState, WarmupTime, MatchTime, LevelStartingTime, CooldownTime, MatchTimeLeftAlert, GameMode->GetIsTeamsMatch());

    if (BlasterHUD && MatchState == MatchState::WaitingToStart)
    {
        BlasterHUD->AddAnnouncementWidget();
    }
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch,  //
    float Warmup,                                                                    //
    float Match,                                                                     //
    float StartingTime,                                                              //
    float TimeOfCooldown,                                                            //
    float TimeOfMatchLeftAlert,                                                      //
    bool bTeamsMatch                                                                 //
)
{
    WarmupTime = Warmup;
    MatchTime = Match;
    LevelStartingTime = StartingTime;
    MatchState = StateOfMatch;
    CooldownTime = TimeOfCooldown;
    MatchTimeLeftAlert = TimeOfMatchLeftAlert;
    OnMatchStateSet(MatchState);

    if (BlasterHUD && MatchState == MatchState::WaitingToStart)
    {
        BlasterHUD->AddAnnouncementWidget();
    }
    if (MatchState == MatchState::InProgress)
    {
        ShowTeamScores(bTeamsMatch);
    }
}

bool ABlasterPlayerController::IsHUDValid()
{
    return BlasterUtils::CastOrUseExistsActor<ABlasterHUD>(BlasterHUD, GetHUD());
}

bool ABlasterPlayerController::IsCharacterOverlayValid()
{
    return IsHUDValid() && BlasterHUD->CharacterOverlay;
}

bool ABlasterPlayerController::IsAnnouncementWidgetValid()
{
    return IsHUDValid() && BlasterHUD->AnnouncementWidget;
}

bool ABlasterPlayerController::IsGameModeValid()
{

    return BlasterUtils::CastOrUseExistsActor(BlasterGameMode, UGameplayStatics::GetGameMode(this));
}

// Is the ping too high?
void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
    HighPingDelegate.Broadcast(bHighPing);
}

bool ABlasterPlayerController::IsBlasterCharacterValid()
{
    BlasterCharacter = !BlasterCharacter.IsValid() ? Cast<ABlasterCharacter>(GetCharacter()) : BlasterCharacter;
    return BlasterCharacter.IsValid();
}

void ABlasterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
    ClientElimAnnouncement(Attacker, Victim);
}

void ABlasterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
    APlayerState* Self = GetPlayerState<APlayerState>();
    if (!Self || !Attacker || !Victim || !IsHUDValid()) return;
    if (Attacker == Self && Victim != Self)
    {
        BlasterHUD->AddElimAnnouncementWidget("You", Victim->GetPlayerName());
        return;
    }
    if (Victim == Self && Attacker != Self)
    {
        BlasterHUD->AddElimAnnouncementWidget(Attacker->GetPlayerName(), "you");
        return;
    }
    // Suicide
    if (Attacker == Victim && Attacker == Self)
    {
        BlasterHUD->AddElimAnnouncementWidget("You", "yourself");
        return;
    }
    // Someone else killed himself
    if (Attacker == Victim && Attacker != Self)
    {
        BlasterHUD->AddElimAnnouncementWidget(Attacker->GetPlayerName(), "themselves");
        return;
    }
    BlasterHUD->AddElimAnnouncementWidget(Attacker->GetPlayerName(), Victim->GetPlayerName());
}

void ABlasterPlayerController::OnRep_ShowTeamScores()
{
    ShowTeamScores(bShowTeamScores);
}

void ABlasterPlayerController::ShowTeamScores(bool bIsVisible)
{
    if (bIsVisible)
    {
        InitTeamScores();
    }
    else
    {
        HideTeamScores();
    }
}