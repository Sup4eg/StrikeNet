// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "BlasterUtils.h"
#include "Net/UnrealNetwork.h"
#include "BlasterCharacter.h"
#include "BlasterPlayerController.h"
#include "CarryItem.h"

ACarryItem::ACarryItem()
{
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    SetReplicateMovement(true);

    ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
    SetRootComponent(ItemMesh);
    ItemMesh->SetupAttachment(AreaSphere);
    ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
    ItemMesh->SetCollisionResponseToChannel(ECC_IK_Visibility, ECollisionResponse::ECR_Ignore);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Area Sphere"));
    AreaSphere->SetupAttachment(RootComponent);
    AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AreaSphere->SetSphereRadius(85.f);

    PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Pickup Widget"));
    PickupWidget->SetupAttachment(RootComponent);
}

void ACarryItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACarryItem, State);
}

void ACarryItem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    RunningTime += DeltaTime;
    if (bIsHovering)
    {
        AddActorWorldOffset(FVector(0.f, 0.f, TransformedSin()));
    }
}

float ACarryItem::TransformedSin()
{
    return Amplitude * FMath::Sin(RunningTime * TimeConstant);
}

void ACarryItem::BeginPlay()
{
    Super::BeginPlay();

    if (PickupWidget)
    {
        PickupWidget->SetVisibility(false);
    }

    AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnsphereOverlap);
    AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);

    if (ItemMesh)
    {
        InitializeMaterials = ItemMesh->GetMaterials();
    }
}

void ACarryItem::OnRep_Owner()
{
    Super::OnRep_Owner();
    if (!GetOwner())
    {
        BlasterOwnerCharacter = nullptr;
        BlasterOwnerController = nullptr;
    }
}

void ACarryItem::OnsphereOverlap(UPrimitiveComponent* OverlappedComponent,  //
    AActor* OtherActor,                                                      //
    UPrimitiveComponent* OtherComp,                                          //
    int32 OtherBodyIndex,                                                    //
    bool bFromSweep,                                                         //
    const FHitResult& SweepResult)
{
}

void ACarryItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,  //
    AActor* OtherActor,                                                         //
    UPrimitiveComponent* OtherComp,                                             //
    int32 OtherBodyIndex)
{
}

void ACarryItem::OnPingTooHigh(bool bPingTooHigh) {}

void ACarryItem::OnRep_State()
{
    if (!ItemMesh) return;

    OnStateSet();
}

void ACarryItem::SetState(ECarryItemState StateToSet)
{
    if (!ItemMesh) return;

    State = StateToSet;
    OnStateSet();
}

void ACarryItem::OnStateSet()
{
    switch (State)
    {
        case ECarryItemState::ECIS_Equipped: OnEquipped(); break;
        case ECarryItemState::ECIS_EquippedSecondary: OnEquippedSecondary(); break;
        case ECarryItemState::ECIS_Dropped: OnDropped(); break;
        default: break;
    }
}

void ACarryItem::OnEquipped()
{
    if (!ItemMesh || !AreaSphere) return;
    ShowPickupWidget(false);
    AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ItemMesh->SetSimulatePhysics(false);
    ItemMesh->SetEnableGravity(false);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACarryItem::OnEquippedSecondary()
{
    if (!ItemMesh) return;
    ShowPickupWidget(false);
    ItemMesh->SetSimulatePhysics(false);
    ItemMesh->SetEnableGravity(false);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACarryItem::OnDropped()
{
    if (HasAuthority() && AreaSphere)
    {
        AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
    if (!ItemMesh) return;

    ItemMesh->SetSimulatePhysics(true);
    ItemMesh->SetEnableGravity(true);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    ItemMesh->SetCollisionResponseToChannel(ECC_IK_Visibility, ECollisionResponse::ECR_Ignore);
    ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
    ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

    if (bIsInvisible)
    {
        SetDefaultMaterial();
        bIsInvisible = false;
    }
}

void ACarryItem::Dropped()
{
    if (!ItemMesh) return;

    SetState(ECarryItemState::ECIS_Dropped);
    FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
    ItemMesh->DetachFromComponent(DetachRules);
    SetOwner(nullptr);
    BlasterOwnerCharacter = nullptr;
    BlasterOwnerController = nullptr;
}

bool ACarryItem::IsBlasterOwnerCharacterValid()
{
    return BlasterUtils::CastOrUseExistsActor(BlasterOwnerCharacter, GetOwner());
}

bool ACarryItem::IsBlasterOwnerControllerValid()
{
    return IsBlasterOwnerCharacterValid() &&
           BlasterUtils::CastOrUseExistsActor(BlasterOwnerController, BlasterOwnerCharacter->GetController());
}

void ACarryItem::SetMaterial(UMaterialInterface* NewMaterial)
{
    if (!ItemMesh) return;
    for (int i = 0; i < InitializeMaterials.Num(); ++i)
    {
        ItemMesh->SetMaterial(i, NewMaterial);
    }
}

void ACarryItem::SetDefaultMaterial()
{
    if (!ItemMesh) return;
    for (int i = 0; i < InitializeMaterials.Num(); ++i)
    {
        ItemMesh->SetMaterial(i, InitializeMaterials[i]);
    }
}

void ACarryItem::ShowPickupWidget(bool bShowWidget)
{
    if (!PickupWidget) return;
    PickupWidget->SetVisibility(bShowWidget);
}