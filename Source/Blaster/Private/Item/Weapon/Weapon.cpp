// Fill out your copyright notice in the Description page of Project Settings.

#include "Net/UnrealNetwork.h"
#include "BlasterCharacter.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Casing.h"
#include "BlasterPlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon.h"

AWeapon::AWeapon()
{
    bReplicates = false;
    PrimaryActorTick.bCanEverTick = true;

    EnableCustomDepth(true);

    if (ItemMesh)
    {
        ItemMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
        ItemMesh->MarkRenderStateDirty();
    }

    if (PickupWidget)
    {
        PickupWidget->AddLocalOffset(FVector(0.f, 0.f, 60.f));
    }
}

void AWeapon::BeginPlay()
{
    Tags.Add("Weapon");
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
    if (!ItemMesh) return;
    ItemMesh->SetRenderCustomDepth(bEnable);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
}

void AWeapon::Fire(const FVector_NetQuantize100& HitTarget, const FVector_NetQuantize100& SocketLocation)
{
    if (!ItemMesh) return;

    if (FireAnimation)
    {
        ItemMesh->PlayAnimation(FireAnimation, false);
    }
    const USkeletalMeshSocket* AmmoEjectSocket = ItemMesh->GetSocketByName(FName("AmmoEject"));
    if (AmmoEjectSocket && GetWorld() && CasingClass)
    {
        FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(ItemMesh);
        GetWorld()->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator());
    }
    SpendRound();
}

void AWeapon::OnsphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    Super::OnsphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
    if (!BlasterCharacter) return;
    BlasterCharacter->SetOverlappingWeapon(this);
}

void AWeapon::OnSphereEndOverlap(
    UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    Super::OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
    if (!BlasterCharacter) return;
    BlasterCharacter->SetOverlappingWeapon(nullptr);
}

void AWeapon::OnEquipped()
{
    Super::OnEquipped();
    if (!ItemMesh) return;
    if (WeaponType == EWeaponType::EWT_SMG)
    {
        ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        ItemMesh->SetEnableGravity(true);
        ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    }

    EnableCustomDepth(false);
    if (HasAuthority() &&                                       //
        bUseServerSideRewindDefault &&                          //
        IsBlasterOwnerControllerValid() &&                      //
        !BlasterOwnerController->HighPingDelegate.IsBound() &&  //
        !BlasterOwnerCharacter->IsLocallyControlled())
    {
        BlasterOwnerController->HighPingDelegate.AddDynamic(this, &ThisClass::OnPingTooHigh);
    }
}

void AWeapon::OnEquippedSecondary()
{
    Super::OnEquippedSecondary();
    if (!ItemMesh) return;
    if (WeaponType == EWeaponType::EWT_SMG)
    {
        ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        ItemMesh->SetEnableGravity(true);
        ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    }
    ItemMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
    ItemMesh->MarkRenderStateDirty();
    EnableCustomDepth(true);

    if (HasAuthority() &&                                      //
        bUseServerSideRewindDefault &&                         //
        IsBlasterOwnerControllerValid() &&                     //
        BlasterOwnerController->HighPingDelegate.IsBound() &&  //
        !BlasterOwnerCharacter->IsLocallyControlled())
    {
        BlasterOwnerController->HighPingDelegate.RemoveDynamic(this, &ThisClass::OnPingTooHigh);
    }
}

void AWeapon::OnDropped()
{
    Super::OnDropped();
    if (!ItemMesh) return;

    ItemMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
    ItemMesh->MarkRenderStateDirty();
    EnableCustomDepth(true);

    if (HasAuthority() &&                                      //
        bUseServerSideRewindDefault &&                         //
        IsBlasterOwnerControllerValid() &&                     //
        BlasterOwnerController->HighPingDelegate.IsBound() &&  //
        !BlasterOwnerCharacter->IsLocallyControlled()          //
    )
    {
        BlasterOwnerController->HighPingDelegate.RemoveDynamic(this, &ThisClass::OnPingTooHigh);
    }
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
    if (GetOwner())
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

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
    Super::OnPingTooHigh(bPingTooHigh);
    bUseServerSideRewind = !bPingTooHigh;
}

bool AWeapon::IsEmpty()
{
    return Ammo <= 0;
}

bool AWeapon::IsFull()
{
    return Ammo == MagCapacity;
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
    if (!ItemMesh) return FVector();
    const USkeletalMeshSocket* MuzzleFlashSocket = ItemMesh->GetSocketByName("MuzzleFlash");
    if (!MuzzleFlashSocket || !GetWorld()) return FVector();
    const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(ItemMesh);
    return SocketTransform.GetLocation();
}