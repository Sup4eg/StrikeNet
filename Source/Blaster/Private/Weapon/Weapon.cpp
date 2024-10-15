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
#include "Kismet/KismetMathLibrary.h"
#include "BlasterUtils.h"
#include "Weapon.h"

AWeapon::AWeapon()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    SetReplicateMovement(true);

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
    SetRootComponent(WeaponMesh);
    WeaponMesh->SetupAttachment(AreaSphere);
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
    AreaSphere->SetSphereRadius(85.f);

    PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Pickup Widget"));
    PickupWidget->SetupAttachment(RootComponent);
    PickupWidget->AddLocalOffset(FVector(0.f, 0.f, 60.f));
}

void AWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (HasAuthority())
    {
        RunningTime += DeltaTime;
        if (bIsHovering)
        {
            AddActorWorldOffset(FVector(0.f, 0.f, TransformedSin()));
        }
    }
}

float AWeapon::TransformedSin()
{
    return Amplitude * FMath::Sin(RunningTime * TimeConstant);
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

    AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnsphereOverlap);
    AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);

    if (GetWeaponMesh())
    {
        InitializeMaterials = GetWeaponMesh()->GetMaterials();
    }
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AWeapon, WeaponState);
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
    Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
    SetHUDAmmo();
    if (HasAuthority())
    {
        ClientUpdateAmmo(Ammo);
    }
    else
    {
        ++Sequence;
    }
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
    if (HasAuthority()) return;
    Ammo = ServerAmmo;
    --Sequence;
    Ammo -= Sequence;
    SetHUDAmmo();
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
    Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
    SetHUDAmmo();
    ClientAddAmmo(AmmoToAdd);
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
    if (HasAuthority()) return;

    Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);

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

    if (IsBlasterOwnerControllerValid())
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

    if (bIsInvisible)
    {
        SetDefaultMaterial();
        bIsInvisible = false;
    }
}

bool AWeapon::IsEmpty()
{
    return Ammo <= 0;
}

bool AWeapon::IsFull()
{
    return Ammo == MagCapacity;
}

void AWeapon::SetMaterial(UMaterialInterface* NewMaterial)
{
    if (!GetWeaponMesh()) return;
    for (int i = 0; i < InitializeMaterials.Num(); ++i)
    {
        GetWeaponMesh()->SetMaterial(i, NewMaterial);
    }
}

void AWeapon::SetDefaultMaterial()
{
    if (!GetWeaponMesh()) return;
    for (int i = 0; i < InitializeMaterials.Num(); ++i)
    {
        GetWeaponMesh()->SetMaterial(i, InitializeMaterials[i]);
    }
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

bool AWeapon::IsBlasterOwnerCharacterValid()
{
    return BlasterUtils::CastOrUseExistsActor(BlasterOwnerCharacter, GetOwner());
}

bool AWeapon::IsBlasterOwnerControllerValid()
{
    return IsBlasterOwnerCharacterValid() &&
           BlasterUtils::CastOrUseExistsActor(BlasterOwnerController, BlasterOwnerCharacter->GetController());
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget, const FVector& TraceStart)
{
    const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
    const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

    const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
    const FVector EndLoc = SphereCenter + RandVec;
    const FVector ToEndLoc = EndLoc - TraceStart;
    /*
    DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
    DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);
    DrawDebugLine(GetWorld(),                                    //
        TraceStart,                                              //
        TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size(),  //
        FColor::Cyan,                                            //
        true);
    */
    return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}

FVector AWeapon::GetTraceStart()
{
    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (!MuzzleFlashSocket || !GetWorld()) return FVector();
    const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
    return SocketTransform.GetLocation();
}