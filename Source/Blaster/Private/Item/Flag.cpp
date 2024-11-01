// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Flag.h"

AFlag::AFlag()
{
    PrimaryActorTick.bCanEverTick = false;
    bIsHovering = false;
    bReplicates = true;

    ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    ItemMesh->SetCollisionResponseToChannel(ECC_IK_Visibility, ECollisionResponse::ECR_Ignore);
    ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    ItemMesh->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Ignore);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ItemMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.8f));

    CanvasMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Canvas Mesh"));
    CanvasMesh->SetupAttachment(ItemMesh);
    CanvasMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CanvasMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.6f));

    PickupWidget->AddLocalOffset(FVector(0.f, 0.f, 230.f));
}

void AFlag::BeginPlay()
{
    Super::BeginPlay();
    Tags.Add("Flag");
}

void AFlag::OnDropped()
{
    Super::OnDropped();
    Super::PlayDropSound();
}