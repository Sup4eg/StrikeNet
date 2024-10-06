// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "BlasterCharacter.h"
#include "Animation/AnimationAsset.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Casing.h"
#include "BlasterPlayerController.h"
#include "BlasterUtils.h"
#include "Weapon.h"

AWeapon::AWeapon()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    SetReplicateMovement(true);

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
    SetRootComponent(WeaponMesh);
    WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
    WeaponMesh->SetCollisionResponseToChannel(ECC_IK_Visibility, ECollisionResponse::ECR_Ignore);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
    WeaponMesh->MarkRenderStateDirty();
    EnableCustomDepth(true);

    AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Area Sphere"));
    AreaSphere->SetupAttachment(RootComponent);
    AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Pickup Widget"));
    PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
    if (!WeaponMesh) return;
    WeaponMesh->SetRenderCustomDepth(bEnable);
}

void AWeapon::BeginPlay()
{
    Super::BeginPlay();

    if (PickupWidget)
    {
        PickupWidget->SetVisibility(false);
    }

    if (HasAuthority())
    {
        AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
        AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnsphereOverlap);
        AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
    }
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AWeapon, WeaponState);
    DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
    if (!PickupWidget) return;
    PickupWidget->SetVisibility(bShowWidget);
}

void AWeapon::Fire(const FVector& HitTarget)
{
    if (!WeaponMesh) return;

    if (FireAnimation)
    {
        WeaponMesh->PlayAnimation(FireAnimation, false);
    }
    const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
    if (AmmoEjectSocket && GetWorld() && CasingClass)
    {
        FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
        GetWorld()->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator());
    }
    SpendRound();
}

void AWeapon::OnsphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
    if (!BlasterCharacter) return;
    BlasterCharacter->SetOverlappingWeapon(this);
}

void AWeapon::OnSphereEndOverlap(
    UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
    if (!BlasterCharacter) return;
    BlasterCharacter->SetOverlappingWeapon(nullptr);
}

void AWeapon::SpendRound()
{
    if (HasAuthority())
    {
        Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
        SetHUDAmmo();
    }
}

void AWeapon::OnRep_Ammo()
{
    if (WeaponType == EWeaponType::EWT_Shotgun && IsBlasterOwnerCharacterValid() && IsFull())
    {
        BlasterOwnerCharacter->PlayMontage(BlasterOwnerCharacter->GetReloadMontage(), FName("ShotgunEnd"));
    }
    SetHUDAmmo();
}

void AWeapon::OnRep_Owner()
{
    Super::OnRep_Owner();
    if (!GetOwner())
    {
        BlasterOwnerCharacter = nullptr;
        BlasterOwnerController = nullptr;
    }
    else
    {
        if (IsBlasterOwnerCharacterValid() && BlasterOwnerCharacter->GetEquippedWeapon() &&
            BlasterOwnerCharacter->GetEquippedWeapon() == this)
        {
            SetHUDAmmo();
        }
    }
}

void AWeapon::SetHUDAmmo()
{

    if (IsBlasterOwnerCharacterValid() &&
        BlasterUtils::CastOrUseExistsActor(BlasterOwnerController, BlasterOwnerCharacter->GetController()))
    {
        BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
    }
}

void AWeapon::OnRep_WeaponState()
{
    if (!WeaponMesh) return;

    OnWeaponStateSet();
}

void AWeapon::SetWeaponState(EWeaponState State)
{
    if (!WeaponMesh) return;

    WeaponState = State;
    OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
    switch (WeaponState)
    {
        case EWeaponState::EWS_Equipped: OnEquipped(); break;
        case EWeaponState::EWS_EquippedSecondary: OnEquippedSecondary(); break;
        case EWeaponState::EWS_Dropped: OnDropped(); break;
        default: break;
    }
}

void AWeapon::OnEquipped()
{
    ShowPickupWidget(false);
    AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->SetSimulatePhysics(false);
    WeaponMesh->SetEnableGravity(false);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (WeaponType == EWeaponType::EWT_SMG)
    {
        WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        WeaponMesh->SetEnableGravity(true);
        WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    }
    EnableCustomDepth(false);
}

void AWeapon::OnEquippedSecondary()
{
    ShowPickupWidget(false);
    WeaponMesh->SetSimulatePhysics(false);
    WeaponMesh->SetEnableGravity(false);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (WeaponType == EWeaponType::EWT_SMG)
    {
        WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        WeaponMesh->SetEnableGravity(true);
        WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    }
    WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
    WeaponMesh->MarkRenderStateDirty();
    EnableCustomDepth(true);
}

void AWeapon::OnDropped()
{
    if (HasAuthority())
    {
        AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
    WeaponMesh->SetSimulatePhysics(true);
    WeaponMesh->SetEnableGravity(true);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    WeaponMesh->SetCollisionResponseToChannel(ECC_IK_Visibility, ECollisionResponse::ECR_Ignore);
    WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
    WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

    WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
    WeaponMesh->MarkRenderStateDirty();
    EnableCustomDepth(true);
}

bool AWeapon::IsEmpty()
{
    return Ammo <= 0;
}

bool AWeapon::IsFull()
{
    return Ammo == MagCapacity;
}

void AWeapon::Dropped()
{
    if (!WeaponMesh) return;

    SetWeaponState(EWeaponState::EWS_Dropped);
    FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
    WeaponMesh->DetachFromComponent(DetachRules);
    SetOwner(nullptr);
    BlasterOwnerCharacter = nullptr;
    BlasterOwnerController = nullptr;
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
    Ammo = FMath::Clamp(Ammo - AmmoToAdd, 0, MagCapacity);
    SetHUDAmmo();
}

bool AWeapon::IsBlasterOwnerCharacterValid()
{
    return BlasterUtils::CastOrUseExistsActor(BlasterOwnerCharacter, GetOwner());
}
