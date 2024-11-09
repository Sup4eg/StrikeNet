// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SphereComponent.h"
#include "Flag.h"
#include "CTFGameMode.h"
#include "Engine/World.h"
#include "FlagZone.h"

AFlagZone::AFlagZone()
{
    PrimaryActorTick.bCanEverTick = false;

    ZoneSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneSphere"));
    SetRootComponent(ZoneSphere);
    ZoneSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ZoneSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
    ZoneSphere->SetCollisionResponseToChannel(ECC_IK_Visibility, ECollisionResponse::ECR_Ignore);
    ZoneSphere->SetGenerateOverlapEvents(true);
}

void AFlagZone::BeginPlay()
{
    Super::BeginPlay();

	ZoneSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnsphereOverlap);
}

void AFlagZone::OnsphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
    AActor* OtherActor,                                                    //
    UPrimitiveComponent* OtherComp,                                        //
    int32 OtherBodyIndex,                                                  //
    bool bFromSweep,                                                       //
    const FHitResult& SweepResult)
{
    if (!GetWorld()) return;
    if (OtherActor && OtherActor->ActorHasTag("Flag"))
    {
        if (AFlag* OverlappingFlag = Cast<AFlag>(OtherActor))
        {
            if (OverlappingFlag->GetTeam() != Team)
            {
                if (ACTFGameMode* CTFGameMode = GetWorld()->GetAuthGameMode<ACTFGameMode>())
                {
                    CTFGameMode->FlagCaptured(OverlappingFlag, this);
					OverlappingFlag->Initialized();
                }
            }
        }
    }
}
