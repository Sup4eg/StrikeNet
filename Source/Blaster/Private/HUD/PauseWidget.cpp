// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "BlasterCharacter.h"
#include "BlasterUtils.h"
#include "BlasterPlayerController.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "PauseWidget.h"

void UPauseWidget::MenuSetup()
{
    AddToViewport();
    SetVisibility(ESlateVisibility::Visible);
    SetIsFocusable(true);

    if (IsBlasterPlayerControllerValid())
    {
        FInputModeGameAndUI InputModeData;
        InputModeData.SetWidgetToFocus(TakeWidget());
        BlasterPlayerController->SetInputMode(InputModeData);
        BlasterPlayerController->SetShowMouseCursor(true);

        if (ABlasterCharacter* PlayerCharacter = Cast<ABlasterCharacter>(BlasterPlayerController->GetCharacter()))
        {
            if (SelfDestructionButton)
            {
                SelfDestructionButton->SetIsEnabled(!PlayerCharacter->GetIsElimmed());
            }
        }
    }

    if (ReturnButton && !ReturnButton->OnClicked.IsBound())
    {
        ReturnButton->OnClicked.AddDynamic(this, &ThisClass::ReturnButtonClicked);
    }

    if (SelfDestructionButton && !SelfDestructionButton->OnClicked.IsBound())
    {
        SelfDestructionButton->OnClicked.AddDynamic(this, &ThisClass::SelfDestructionButtonClicked);
    }

    if (OpenSound && GetWorld())
    {
        UGameplayStatics::PlaySound2D(GetWorld(), OpenSound);
    }
}

bool UPauseWidget::Initialize()
{
    if (!Super::Initialize()) return false;
    if (IsBlasterPlayerControllerValid())
    {
        BlasterPlayerController->OnPlayerCharacterBeginPlay.AddUObject(this, &ThisClass::PlayerCharacterBeginPlay);
    }
    if (GetGameInstance())
    {
        MultiplayerSessionsSubsystem = GetGameInstance()->GetSubsystem<UMultiplayerSessionsSubsystem>();
        if (MultiplayerSessionsSubsystem)
        {
            MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
        }
    }
    return true;
}

void UPauseWidget::OnDestroySession(bool bWasSuccessful)
{
    if (!bWasSuccessful && ReturnButton)
    {
        ReturnButton->SetIsEnabled(true);
        return;
    }

    if (!GetWorld()) return;
    AGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AGameModeBase>();
    // Debug
    // if (GEngine)
    // {
    //     GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Try to shutdown session!!!"));
    // }
    if (GameMode)
    {
        GameMode->ReturnToMainMenuHost();
    }
    else if (IsBlasterPlayerControllerValid())
    {
        BlasterPlayerController->ClientReturnToMainMenuWithTextReason(FText());
    }
}

void UPauseWidget::MenuTearDown()
{
    RemoveFromParent();

    if (IsBlasterPlayerControllerValid())
    {
        FInputModeGameOnly InputModeData;
        BlasterPlayerController->SetInputMode(InputModeData);
        BlasterPlayerController->SetShowMouseCursor(false);
    }

    if (ReturnButton && ReturnButton->OnClicked.IsBound())
    {
        ReturnButton->OnClicked.RemoveDynamic(this, &ThisClass::ReturnButtonClicked);
    }

    if (SelfDestructionButton && SelfDestructionButton->OnClicked.IsBound())
    {
        SelfDestructionButton->OnClicked.RemoveDynamic(this, &ThisClass::SelfDestructionButtonClicked);
    }
}

void UPauseWidget::ReturnButtonClicked()
{
    ReturnButton->SetIsEnabled(false);

    if (IsBlasterPlayerControllerValid())
    {
        if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(BlasterPlayerController->GetCharacter()))
        {
            BlasterCharacter->ServerLeaveGame();
            BlasterCharacter->OnLeftGame.AddDynamic(this, &ThisClass::OnPlayerLeftGame);
        }
        else
        {
            ReturnButton->SetIsEnabled(true);
        }
    }
}

void UPauseWidget::OnPlayerLeftGame()
{

    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->DestroySession();
    }
}

void UPauseWidget::SelfDestructionButtonClicked()
{
    SelfDestructionButton->SetIsEnabled(false);
    if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(BlasterPlayerController->GetCharacter()))
    {
        BlasterCharacter->ServerSelfDestruction();
    }
}

void UPauseWidget::PlayerCharacterBeginPlay()
{
    if (SelfDestructionButton)
    {
        SelfDestructionButton->SetIsEnabled(true);
    }
}

bool UPauseWidget::IsBlasterPlayerControllerValid()
{
    if (!GetWorld()) return false;
    return BlasterUtils::CastOrUseExistsActor<ABlasterPlayerController>(BlasterPlayerController, GetWorld()->GetFirstPlayerController());
}
