// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/ShapeComponent.h"
#include "BlasterGameplayStatics.h"
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
    UBlasterGameplayStatics::SelfDestruction(OtherActor, FName("BlasterCharacter"));
}
