// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "BlasterCharacter.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "Flag.h"

AFlag::AFlag()
{
    PrimaryActorTick.bCanEverTick = true;
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

    DropEffect = CreateDefaultSubobject<UNiagaraComponent>("Embers");
    DropEffect->SetupAttachment(GetRootComponent());

    PickupWidget->AddLocalOffset(FVector(0.f, 0.f, 230.f));
}

void AFlag::BeginPlay()
{
    Super::BeginPlay();
    Tags.Add("Flag");
    InitialTransform = GetTransform();
    if (DropEffect)
    {
        DropEffect->Deactivate();
    }
}

void AFlag::OnDropped()
{
    Super::OnDropped();
    Super::PlayDropSound();
    if (!ItemMesh) return;
    ItemMesh->SetSimulatePhysics(false);
    SetIsHovering(true);
    bReplicates = false;
    SetReplicateMovement(false);

    if (HasAuthority())
    {
        SetReturnToBaseTimer();
    }
}

void AFlag::Initialized()
{
    Super::Initialized();
    SetState(ECarryItemState::ECIS_Initial);
    if (IsBlasterOwnerCharacterValid())
    {
        BlasterOwnerCharacter->SetFlag(nullptr);
        if (BlasterOwnerCharacter->IsLocallyControlled())
        {
            BlasterOwnerCharacter->UnCrouch();
        }
    }
    SetOwner(nullptr);
    BlasterOwnerCharacter = nullptr;
    BlasterOwnerController = nullptr;
    SetActorTransform(InitialTransform);
}

void AFlag::OnInitialized()
{
    Super::OnInitialized();
    if (!ItemMesh) return;
    bReplicates = true;
    SetReplicateMovement(true);
    SetIsHovering(false);
    FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
    ItemMesh->DetachFromComponent(DetachRules);

    ShowPickupWidget(false);
    ItemMesh->SetSimulatePhysics(false);
    ItemMesh->SetEnableGravity(false);
    ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    ItemMesh->SetCollisionResponseToChannel(ECC_IK_Visibility, ECollisionResponse::ECR_Ignore);
    ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    ItemMesh->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Ignore);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    if (HasAuthority() && AreaSphere)
    {
        AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }

    if (DropEffect)
    {
        DropEffect->Deactivate();
    }

    if (HasAuthority())
    {
        GetWorldTimerManager().ClearTimer(ReternToBaseTimer);
    }
}

void AFlag::OnEquipped()
{
    Super::OnEquipped();
    SetIsHovering(false);
    bReplicates = true;
    SetReplicateMovement(true);
    if (!ItemMesh) return;
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Ignore);
    ItemMesh->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Ignore);
    ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
    ItemMesh->SetGenerateOverlapEvents(true);
    if (DropEffect)
    {
        DropEffect->Activate();
    }
    if (HasAuthority())
    {
        GetWorldTimerManager().ClearTimer(ReternToBaseTimer);
    }
}

void AFlag::OnsphereOverlap(                   //
    UPrimitiveComponent* OverlappedComponent,  //
    AActor* OtherActor,                        //
    UPrimitiveComponent* OtherComp,            //
    int32 OtherBodyIndex,                      //
    bool bFromSweep,                           //
    const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor->ActorHasTag("BlasterCharacter"))
    {
        if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
        {
            if (BlasterCharacter->GetTeam() != Team)
            {
                BlasterCharacter->SetOverlappingCarryItem(this);
            }
        }
    }
}

void AFlag::OnSphereEndOverlap(                //
    UPrimitiveComponent* OverlappedComponent,  //
    AActor* OtherActor,                        //
    UPrimitiveComponent* OtherComp,            //
    int32 OtherBodyIndex)
{
    if (OtherActor && OtherActor->ActorHasTag("BlasterCharacter"))
    {
        if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
        {
            if (BlasterCharacter->GetTeam() != Team)
            {
                BlasterCharacter->SetOverlappingCarryItem(nullptr);
            }
        }
    }
}
