// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/ShapeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "BlasterCharacter.h"
#include "BlasterGameMode.h"
#include "DeathVolume.h"

ADeathVolume::ADeathVolume()
{
    bReplicates = true;

    GetCollisionComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GetCollisionComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
    GetCollisionComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    GetCollisionComponent()->SetGenerateOverlapEvents(true);
}

void ADeathVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
    Super::NotifyActorBeginOverlap(OtherActor);

    if (HasAuthority() && OtherActor->ActorHasTag("BlasterCharacter"))
    {
        if (ABlasterCharacter* CharacterToKill = Cast<ABlasterCharacter>(OtherActor))
        {
            if (ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)))
            {
                if (!CharacterToKill->IsControllerValid()) return;

                GameMode->PlayerElimmed(
                    CharacterToKill, CharacterToKill->GetBlasterPlayerController(), CharacterToKill->GetBlasterPlayerController());
            }
        }
    }
}
