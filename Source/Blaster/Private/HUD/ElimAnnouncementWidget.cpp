// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/TextBlock.h"
#include "ElimAnnouncementWidget.h"

void UElimAnnouncementWidget::SetElimAnnouncementText(FString AttackerName, FString VictimName)
{
    if (KillerNameText && VictimNameText)
    {
        KillerNameText->SetText(FText::FromString(AttackerName));
        VictimNameText->SetText(FText::FromString(VictimName));
    }
}
