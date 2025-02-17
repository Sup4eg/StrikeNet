// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "BlasterCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "BlasterUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "Engine/World.h"
#include "Weapon.h"
#include "Blaster.h"
#include "BlasterGameplayStatics.h"
#include "HitScanWeapon.h"
#include "ProjectileWeapon.h"
#include "Projectile.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "LagCompensationComponent.h"

ULagCompensationComponent::ULagCompensationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
    Super::BeginPlay();
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    SaveFramePackage();
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
    for (auto& BoxInfo : Package.HitBoxInfo)
    {
        DrawDebugBox(GetWorld(), BoxInfo.Value.Location, BoxInfo.Value.BoxExtent, FQuat(BoxInfo.Value.Rotation), Color, false, 4.f);
    }
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter,  //
    const FVector_NetQuantize& TraceStart,                                                            //
    const FVector_NetQuantize100& HitLocation,                                                        //
    float HitTime)
{
    FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
    return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter,  //
    const FVector_NetQuantize& TraceStart,                                                                      //
    const FVector_NetQuantize100& InitialVelocity,                                                              //
    float GravityScale,                                                                                         //
    float HitTime)
{
    FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
    return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, GravityScale, HitTime);
}

FExplosionProjectileServerSideRewindResult ULagCompensationComponent::ExplosionProjectileServerSideRewind(
    const TArray<ABlasterCharacter*>& HitCharacters,  //
    const FVector_NetQuantize& TraceStart,            //
    const FVector_NetQuantize100& InitialVelocity,    //
    float GravityScale,                               //
    float DamageOuterRadius,                          //
    float HitTime)
{
    TArray<FFramePackage> FramesToCheck;
    for (ABlasterCharacter* HitCharacter : HitCharacters)
    {
        FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
    }
    return ExplosionProjectileConfirmHit(FramesToCheck, TraceStart, InitialVelocity, GravityScale, DamageOuterRadius, HitTime);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters,
    const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize100>& HitLocations, float HitTime)
{
    TArray<FFramePackage> FramesToCheck;
    for (ABlasterCharacter* HitCharacter : HitCharacters)
    {
        FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
    }
    return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
    bool bReturn = HitCharacter == nullptr ||                                                         //
                   HitCharacter->GetLagCompensationComponent() == nullptr ||                          //
                   HitCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() == nullptr ||  //
                   HitCharacter->GetLagCompensationComponent()->FrameHistory.GetTail() == nullptr;    //

    if (bReturn) return FFramePackage();

    // Frame package that we check to verify a hit
    FFramePackage FrameToCheck;
    bool bShouldInterpolate = true;
    // Frame history of the HitCharacter
    const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensationComponent()->FrameHistory;
    const float OldestHistoryTime = History.GetTail()->GetValue().Time;
    const float NewestHistoryTime = History.GetHead()->GetValue().Time;
    if (OldestHistoryTime > HitTime)
    {
        // too far back - too laggy to do SSR
        return FFramePackage();
    }
    if (FMath::IsNearlyEqual(OldestHistoryTime, HitTime))
    {
        FrameToCheck = History.GetTail()->GetValue();
        bShouldInterpolate = false;
    }
    if (NewestHistoryTime <= HitTime)
    {
        FrameToCheck = History.GetHead()->GetValue();
        bShouldInterpolate = false;
    }
    TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
    TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
    while (Older->GetValue().Time > HitTime)  // Is older still younger than HitTime ?
    {
        // March back until: OlderTime < YoungerTime < YoungerTime
        if (!Older->GetNextNode()) break;
        Older = Older->GetNextNode();
        if (Older->GetValue().Time > HitTime)
        {
            Younger = Older;
        }
    }
    if (FMath::IsNearlyEqual(Older->GetValue().Time, HitTime))  // highly unlikely
    {
        FrameToCheck = Older->GetValue();
        bShouldInterpolate = false;
    }
    if (bShouldInterpolate)
    {
        // interpolate frames between Younger and Older
        FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
    }
    FrameToCheck.Character = HitCharacter;
    return FrameToCheck;
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(  //
    const FFramePackage& OlderFrame,                           //
    const FFramePackage& YoungerFrame,                         //
    float HitTime)
{
    const float Distance = YoungerFrame.Time - OlderFrame.Time;
    const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

    FFramePackage InterpFramePackage;
    InterpFramePackage.Time = HitTime;

    // Interpolate hit boxes
    for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
    {
        const FName& BoxInfoName = YoungerPair.Key;
        if (OlderFrame.HitBoxInfo.Contains(BoxInfoName) && YoungerFrame.HitBoxInfo.Contains(BoxInfoName))
        {
            const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
            const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];

            FBoxInformation InterpBoxInfo;
            InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
            InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
            InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

            InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
        }
    }
    return InterpFramePackage;
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter,  //
    const FVector_NetQuantize& TraceStart,                                                          //
    const FVector_NetQuantize100& HitLocation,                                                      //
    float HitTime,                                                                                  //
    float Damage,                                                                                   //
    AWeapon* DamageCauser                                                                           //
)
{
    if (!BlasterCharacter || !HitCharacter || !DamageCauser) return;
    FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);
    if (Confirm.bHitConfirmed)
    {
        UGameplayStatics::ApplyDamage(          //
            HitCharacter,                       //
            Damage * Confirm.DamageModifier,    //
            BlasterCharacter->GetController(),  //
            DamageCauser,                       //
            UDamageType::StaticClass()          //
        );
    }
}

bool ULagCompensationComponent::ServerScoreRequest_Validate(ABlasterCharacter* HitCharacter,  //
    const FVector_NetQuantize& TraceStart,                                                    //
    const FVector_NetQuantize100& HitLocation,                                                //
    float HitTime,                                                                            //
    float Damage,                                                                             //
    AWeapon* DamageCauser)
{
    bool bIsValid = false;
    if (AHitScanWeapon* HitScanWeapon = Cast<AHitScanWeapon>(DamageCauser))
    {
        bool bIsRightWeapon = HitScanWeapon->GetWeaponType() == EWeaponType::EWT_Pistol ||  //
                              HitScanWeapon->GetWeaponType() == EWeaponType::EWT_SMG ||     //
                              HitScanWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;

        if (bIsRightWeapon)
        {
            // Validate
            bIsValid = FMath::IsNearlyEqual(Damage, HitScanWeapon->GetDamage(), 0.0001f);
        }
    }

    return bIsValid;
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter,  //
    const FVector_NetQuantize& TraceStart,                                                                    //
    const FVector_NetQuantize100& InitialVelocity,                                                            //
    float GravityScale,                                                                                       //
    float HitTime,                                                                                            //
    float Damage,                                                                                             //
    AWeapon* DamageCauser                                                                                     //
)
{
    if (!BlasterCharacter || !HitCharacter || !DamageCauser) return;
    FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, GravityScale, HitTime);
    if (Confirm.bHitConfirmed)
    {
        UGameplayStatics::ApplyDamage(          //
            HitCharacter,                       //
            Damage * Confirm.DamageModifier,    //
            BlasterCharacter->GetController(),  //
            DamageCauser,                       //
            UDamageType::StaticClass()          //
        );
    }
}

bool ULagCompensationComponent::ProjectileServerScoreRequest_Validate(ABlasterCharacter* HitCharacter,  //
    const FVector_NetQuantize& TraceStart,                                                              //
    const FVector_NetQuantize100& InitialVelocity,                                                      //
    float GravityScale,                                                                                 //
    float HitTime,                                                                                      //
    float Damage,                                                                                       //
    AWeapon* DamageCauser                                                                               //
)
{
    bool bIsValid = false;
    if (AProjectileWeapon* ProjectileWeapon = Cast<AProjectileWeapon>(DamageCauser))
    {
        bool bIsRightWeapon = ProjectileWeapon->GetWeaponType() == EWeaponType::EWT_AssaultRifle;
        if (bIsRightWeapon && ProjectileWeapon->GetProjectileClass())
        {
            float Epsilon = 0.0001f;
            UClass* ProjectileClass = ProjectileWeapon->GetProjectileClass().Get();
            AProjectile* DefaultProjectile = ProjectileClass->GetDefaultObject<AProjectile>();
            bool bIsRightProjectile = DefaultProjectile->GetProjectileType() == EProjectileType::EPT_ProjectileBullet;
            if (bIsRightProjectile)
            {
                // Validate
                bIsValid = FMath::IsNearlyEqual(Damage, DefaultProjectile->GetDamage(), Epsilon) &&         //
                           FMath::IsNearlyEqual(GravityScale, DefaultProjectile->GravityScale, Epsilon) &&  //
                           FMath::IsNearlyEqual(InitialVelocity.Size(), DefaultProjectile->InitialSpeed, 5.f);
            }
        }
    }

    return bIsValid;
}

void ULagCompensationComponent::ExplosionProjectileServerScoreRequest_Implementation(  //
    const TArray<ABlasterCharacter*>& HitCharacters,                                   //
    const FVector_NetQuantize& TraceStart,
    const FVector_NetQuantize100& InitialVelocity,  //
    float GravityScale,                             //
    float Damage,                                   //
    float DamageInnerRadius,                        //
    float DamageOuterRadius,                        //
    AWeapon* DamageCauser,
    float HitTime  //
)
{
    if (!BlasterCharacter || !DamageCauser || HitCharacters.IsEmpty()) return;

    FExplosionProjectileServerSideRewindResult Confirm =
        ExplosionProjectileServerSideRewind(HitCharacters, TraceStart, InitialVelocity, GravityScale, DamageOuterRadius, HitTime);

    UBlasterGameplayStatics::MakeRadialDamageWithFallOff(  //
        Confirm.OverlapCharactersMap,                      //
        TArray<AActor*>(HitCharacters),                    //
        Confirm.Origin,                                    //
        Damage,                                            //
        DamageInnerRadius,                                 //
        DamageOuterRadius,                                 //
        BlasterCharacter->GetController(),                 //
        DamageCauser);
}

bool ULagCompensationComponent::ExplosionProjectileServerScoreRequest_Validate(  //
    const TArray<ABlasterCharacter*>& HitCharacters,                             //
    const FVector_NetQuantize& TraceStart,
    const FVector_NetQuantize100& InitialVelocity,  //
    float GravityScale,                             //
    float Damage,                                   //
    float DamageInnerRadius,                        //
    float DamageOuterRadius,                        //
    AWeapon* DamageCauser,
    float HitTime  //
)
{
    bool bIsValid = false;
    if (AProjectileWeapon* ProjectileWeapon = Cast<AProjectileWeapon>(DamageCauser))
    {
        bool bIsRightWeapon = ProjectileWeapon->GetWeaponType() == EWeaponType::EWT_RocketLauncher;
        if (bIsRightWeapon && ProjectileWeapon->GetProjectileClass())
        {
            float Epsilon = 0.0001f;
            UClass* ProjectileClass = ProjectileWeapon->GetProjectileClass().Get();
            AProjectile* DefaultProjectile = ProjectileClass->GetDefaultObject<AProjectile>();
            bool bIsRightProjectile = DefaultProjectile->GetProjectileType() == EProjectileType::EPT_ProjectileRocket;

            if (bIsRightProjectile)
            {
                // Validate
                bIsValid = FMath::IsNearlyEqual(Damage, DefaultProjectile->GetDamage(), Epsilon) &&                        //
                           FMath::IsNearlyEqual(DamageInnerRadius, DefaultProjectile->GetDamageInnerRadius(), Epsilon) &&  //
                           FMath::IsNearlyEqual(DamageOuterRadius, DefaultProjectile->GetDamageOuterRadius(), Epsilon) &&  //
                           FMath::IsNearlyEqual(GravityScale, DefaultProjectile->GravityScale, Epsilon) &&                 //
                           FMath::IsNearlyEqual(InitialVelocity.Size(), DefaultProjectile->InitialSpeed, 5.f);             //
            }
        }
    }

    return bIsValid;
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters,  //
    const FVector_NetQuantize& TraceStart,                                                                                 //
    const TArray<FVector_NetQuantize100>& HitLocations,                                                                    //
    float HitTime,                                                                                                         //
    float Damage,
    AWeapon* DamageCauser  //
)
{
    if (HitCharacters.IsEmpty() || HitLocations.IsEmpty() || !DamageCauser || !BlasterCharacter) return;
    FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

    for (auto& HitCharacter : HitCharacters)
    {
        if (HitCharacter && Confirm.Shots.Contains(HitCharacter))
        {
            float MultipleDamage = Confirm.Shots[HitCharacter] * Damage;
            UGameplayStatics::ApplyDamage(          //
                HitCharacter,                       //
                MultipleDamage,                     //
                BlasterCharacter->GetController(),  //
                DamageCauser,                       //
                UDamageType::StaticClass()          //
            );
        }
    }
}

bool ULagCompensationComponent::ShotgunServerScoreRequest_Validate(const TArray<ABlasterCharacter*>& HitCharacters,  //
    const FVector_NetQuantize& TraceStart,                                                                           //
    const TArray<FVector_NetQuantize100>& HitLocations,                                                              //
    float HitTime,                                                                                                   //
    float Damage,                                                                                                    //
    AWeapon* DamageCauser)
{
    bool bIsValid = false;
    if (AHitScanWeapon* HitScanWeapon = Cast<AHitScanWeapon>(DamageCauser))
    {
        bool bIsRightWeapon = HitScanWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun;
        if (bIsRightWeapon)
        {
            // Validate
            bIsValid = FMath::IsNearlyEqual(Damage, HitScanWeapon->GetDamage(), 0.0001f);
        }
    }

    return bIsValid;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter,
    const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& HitLocation)
{
    FServerSideRewindResult ServerSideRewindResult;
    ServerSideRewindResult.bHitConfirmed = false;

    if (!HitCharacter) return ServerSideRewindResult;

    FFramePackage CurrentFrame;
    CacheBoxPositions(HitCharacter, CurrentFrame);
    MoveBoxes(HitCharacter, Package);

    EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

    // EnableCollision for hit boxes
    for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
    {
        if (HitBoxPair.Value)
        {
            HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

            // Debug purpose
            // UE_LOG(LogTemp, Warning, TEXT("Component Location: %s"), *HitBoxPair.Value->GetComponentLocation().ToString());
            // UE_LOG(LogTemp, Warning, TEXT("Scaled Box Extent: %s"), *HitBoxPair.Value->GetScaledBoxExtent().ToString());
            // UE_LOG(LogTemp, Warning, TEXT("Component rotation: %s"), *HitBoxPair.Value->GetComponentRotation().ToString());
            // DrawDebugBox(GetWorld(), HitBoxPair.Value->GetComponentLocation(), HitBoxPair.Value->GetUnscaledBoxExtent(),
            //  FQuat(HitBoxPair.Value->GetComponentRotation()), FColor::Red, false, 4.f);
        }
    }

    FHitResult ConfirmHitResult;
    const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
    UWorld* World = GetWorld();

    if (World)
    {
        FCollisionQueryParams Params;
        Params.bReturnPhysicalMaterial = true;

        World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox, Params);
        if (ConfirmHitResult.bBlockingHit && ConfirmHitResult.PhysMaterial.IsValid())
        {
            UPhysicalMaterial* PhysMat = ConfirmHitResult.PhysMaterial.Get();
            if (HitCharacter->DamageModifiers.Contains(PhysMat))
            {
                ServerSideRewindResult.bHitConfirmed = true;
                ServerSideRewindResult.DamageModifier = HitCharacter->DamageModifiers[PhysMat];
            }
        }
    }
    ResetHitBoxes(HitCharacter, CurrentFrame);
    EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

    return ServerSideRewindResult;
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package,  //
    ABlasterCharacter* HitCharacter,                                                                   //
    const FVector_NetQuantize& TraceStart,                                                             //
    const FVector_NetQuantize100& InitialVelocity,                                                     //
    float GravityScale,                                                                                //
    float HitTime)
{

    FServerSideRewindResult ServerSideRewindResult;
    ServerSideRewindResult.bHitConfirmed = false;

    if (!HitCharacter || !GetWorld()) return ServerSideRewindResult;

    FFramePackage CurrentFrame;
    CacheBoxPositions(HitCharacter, CurrentFrame);
    MoveBoxes(HitCharacter, Package);
    EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

    // EnableCollision for hit boxes
    for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
    {
        if (HitBoxPair.Value)
        {
            HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
        }
    }

    FPredictProjectilePathParams PathParams;
    PathParams.bTraceWithChannel = true;
    PathParams.bTraceWithCollision = true;
    PathParams.MaxSimTime = MaxRecordTime;
    PathParams.LaunchVelocity = InitialVelocity;
    PathParams.StartLocation = TraceStart;
    PathParams.OverrideGravityZ = GetWorld()->GetGravityZ() * GravityScale;
    PathParams.SimFrequency = 15.f;
    PathParams.ProjectileRadius = 5.f;
    PathParams.TraceChannel = ECC_HitBox;
    PathParams.ActorsToIgnore.Add(GetOwner());
    PathParams.DrawDebugTime = 5.f;
    PathParams.DrawDebugType = EDrawDebugTrace::None;

    FPredictProjectilePathResult PathResult;
    UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

    if (PathResult.HitResult.bBlockingHit)
    {
        UPrimitiveComponent* HitBoxComponent = PathResult.HitResult.GetComponent();
        if (HitBoxComponent && HitBoxComponent->GetBodyInstance() && HitBoxComponent->GetBodyInstance()->GetSimplePhysicalMaterial())
        {
            UPhysicalMaterial* PhysMat = HitBoxComponent->GetBodyInstance()->GetSimplePhysicalMaterial();
            if (HitCharacter->DamageModifiers.Contains(PhysMat))
            {
                ServerSideRewindResult.bHitConfirmed = true;
                ServerSideRewindResult.DamageModifier = HitCharacter->DamageModifiers[PhysMat];
            }
        }
    }

    ResetHitBoxes(HitCharacter, CurrentFrame);
    EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

    return ServerSideRewindResult;
}

FExplosionProjectileServerSideRewindResult ULagCompensationComponent::ExplosionProjectileConfirmHit(
    const TArray<FFramePackage>& FramePackages,  //
    const FVector_NetQuantize& TraceStart,
    const FVector_NetQuantize100& InitialVelocity,  //
    float GravityScale,                             //
    float DamageOuterRadius,                        //
    float HitTime                                   //
)
{
    FExplosionProjectileServerSideRewindResult ExplosionProjectileResult;
    if (!GetWorld()) return ExplosionProjectileResult;

    TArray<FFramePackage> CurrentFrames;
    // Cache
    for (auto& Frame : FramePackages)
    {
        FFramePackage CurrentFrame;
        CacheBoxPositions(Frame.Character, CurrentFrame);
        MoveBoxes(Frame.Character, Frame);
        EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
        CurrentFrame.Character = Frame.Character;
        CurrentFrames.Add(CurrentFrame);
    }

    // EnableCollision for hit boxes
    for (auto& Frame : FramePackages)
    {
        if (Frame.Character)
        {
            for (auto& HitBoxPair : Frame.Character->HitCollisionBoxes)
            {
                if (HitBoxPair.Value)
                {
                    HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                    HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Overlap);
                    HitBoxPair.Value->SetGenerateOverlapEvents(true);
                }
            }
        }
    }

    FPredictProjectilePathParams PathParams;
    PathParams.bTraceWithChannel = true;
    PathParams.bTraceWithCollision = true;
    PathParams.MaxSimTime = MaxRecordTime;
    PathParams.LaunchVelocity = InitialVelocity;
    PathParams.OverrideGravityZ = GetWorld()->GetGravityZ() * GravityScale;
    PathParams.StartLocation = TraceStart;
    PathParams.SimFrequency = 15.f;
    PathParams.ProjectileRadius = 5.f;
    PathParams.TraceChannel = ECollisionChannel::ECC_WorldStatic;
    PathParams.ActorsToIgnore.Add(GetOwner());
    PathParams.DrawDebugTime = 5.f;
    PathParams.DrawDebugType = EDrawDebugTrace::None;

    FPredictProjectilePathResult PathResult;
    UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

    if (PathResult.HitResult.bBlockingHit)
    {
        ExplosionProjectileResult.Origin = PathResult.HitResult.ImpactPoint;

        UBlasterGameplayStatics::GetOverlapActorsBySphereTrace(this, ExplosionProjectileResult.OverlapCharactersMap,
            PathResult.HitResult.ImpactPoint, FName("BlasterCharacter"), ECC_HitBox, DamageOuterRadius);

        // Debug
        // DrawDebugSphere(GetWorld(), PathResult.HitResult.ImpactPoint, DamageOuterRadius, 20, FColor::Blue, true);
    }

    for (auto& Frame : CurrentFrames)
    {
        ResetHitBoxes(Frame.Character, Frame);
        EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
    }

    return ExplosionProjectileResult;
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(  //
    const TArray<FFramePackage>& FramePackages,                               //
    const FVector_NetQuantize& TraceStart,                                    //
    const TArray<FVector_NetQuantize100>& HitLocations)
{
    FShotgunServerSideRewindResult ShotgunResult;

    if (!GetWorld()) return ShotgunResult;

    TArray<FFramePackage> CurrentFrames;
    // Cache
    for (auto& Frame : FramePackages)
    {
        FFramePackage CurrentFrame;
        CacheBoxPositions(Frame.Character, CurrentFrame);
        MoveBoxes(Frame.Character, Frame);
        EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
        CurrentFrame.Character = Frame.Character;
        CurrentFrames.Add(CurrentFrame);
    }

    // EnableCollision for hit boxes
    for (auto& Frame : FramePackages)
    {
        if (Frame.Character)
        {
            for (auto& HitBoxPair : Frame.Character->HitCollisionBoxes)
            {
                if (HitBoxPair.Value)
                {
                    HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                    HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
                }
            }
        }
    }

    for (auto& HitLocation : HitLocations)
    {
        FHitResult ConfirmHitResult;
        FCollisionQueryParams Params;
        Params.bReturnPhysicalMaterial = true;
        const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
        GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox, Params);
        bool bHitCharacter = ConfirmHitResult.bBlockingHit &&  //
                             ConfirmHitResult.GetActor() &&    //
                             ConfirmHitResult.GetActor()->ActorHasTag("BlasterCharacter");
        if (bHitCharacter && ConfirmHitResult.PhysMaterial.IsValid())
        {
            if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor()))
            {
                UPhysicalMaterial* PhysMat = ConfirmHitResult.PhysMaterial.Get();
                if (HitCharacter->DamageModifiers.Contains(PhysMat))
                {
                    if (ShotgunResult.Shots.Contains(HitCharacter))
                    {
                        ShotgunResult.Shots[HitCharacter] += HitCharacter->DamageModifiers[PhysMat];
                    }
                    else
                    {
                        ShotgunResult.Shots.Emplace(HitCharacter, HitCharacter->DamageModifiers[PhysMat]);
                    }
                }
            }
        }
    }

    for (auto& Frame : CurrentFrames)
    {
        ResetHitBoxes(Frame.Character, Frame);
        EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
    }
    return ShotgunResult;
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
    if (!HitCharacter) return;
    for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
    {
        if (HitBoxPair.Value)
        {
            FBoxInformation BoxInfo;
            BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
            BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
            BoxInfo.BoxExtent = HitBoxPair.Value->GetUnscaledBoxExtent();

            OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
        }
    }
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
    if (!HitCharacter) return;
    for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
    {
        if (HitBoxPair.Value && Package.HitBoxInfo.Contains(HitBoxPair.Key))
        {
            HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
            HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
            HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
        }
    }
}

void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
    if (!HitCharacter) return;
    for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
    {
        if (HitBoxPair.Value && Package.HitBoxInfo.Contains(HitBoxPair.Key))
        {
            HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
            HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
            HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);

            HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
    if (!HitCharacter || !HitCharacter->GetMesh()) return;
    HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
}

void ULagCompensationComponent::SaveFramePackage()
{
    if (!BlasterCharacter || !BlasterCharacter->HasAuthority()) return;
    if (FrameHistory.Num() <= 1)
    {
        FFramePackage ThisFrame;
        SaveFramePackage(ThisFrame);
        FrameHistory.AddHead(ThisFrame);
    }
    else
    {
        float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
        while (HistoryLength > MaxRecordTime)
        {
            FrameHistory.RemoveNode(FrameHistory.GetTail());
            HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
        }
        FFramePackage ThisFrame;
        SaveFramePackage(ThisFrame);
        FrameHistory.AddHead(ThisFrame);

        // Debug purpose
        //ShowFramePackage(ThisFrame, FColor::Red);
    }
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
    if (!IsCharacterValid()) return;
    Package.Time = GetWorld()->GetTimeSeconds();
    Package.Character = BlasterCharacter;
    for (auto& BoxPair : BlasterCharacter->HitCollisionBoxes)
    {
        FBoxInformation BoxInformation;
        BoxInformation.Location = BoxPair.Value->GetComponentLocation();
        BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
        BoxInformation.BoxExtent = BoxPair.Value->GetUnscaledBoxExtent();

        Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
    }
}

bool ULagCompensationComponent::IsCharacterValid()
{
    return BlasterUtils::CastOrUseExistsActor<ABlasterCharacter>(BlasterCharacter, GetOwner());
}
