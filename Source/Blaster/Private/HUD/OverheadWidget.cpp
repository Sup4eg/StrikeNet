// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "OverheadWidget.h"

void UOverheadWidget::SetDisplayText(const FString& TextToDisplay)
{
    if (!DisplayText) return;
    DisplayText->SetText(FText::FromString(TextToDisplay));
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
    ENetRole LocalRole = InPawn->GetLocalRole();
    FString LocalRoleString = FString::Printf(TEXT("Local Role: %s"), *GetLocalRoleString(LocalRole));
    SetDisplayText(LocalRoleString);
}

void UOverheadWidget::NativeDestruct()
{
    Super::NativeDestruct();
    RemoveFromParent();
}

FString UOverheadWidget::GetLocalRoleString(ENetRole LocalRole)
{
    FString Role;
    switch (LocalRole)
    {
        case ENetRole::ROLE_Authority: Role = "Authority"; break;
        case ENetRole::ROLE_AutonomousProxy: Role = "Autonomous Proxy"; break;
        case ENetRole::ROLE_SimulatedProxy: Role = "Simulated Proxy"; break;
        default: Role = "None"; break;
    }
    return Role;
}
