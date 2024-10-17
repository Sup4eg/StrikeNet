// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
class AWeapon;

USTRUCT(BlueprintType)
struct FBoxInformation
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    FVector Location;

    UPROPERTY()
    FRotator Rotation;

    UPROPERTY()
    FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY();
    float Time;

    UPROPERTY()
    TMap<FName, FBoxInformation> HitBoxInfo;

    UPROPERTY();
    ABlasterCharacter* Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    bool bHitConfirmed;
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    TMap<ABlasterCharacter*, uint32> Shots;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULagCompensationComponent();
    friend class ABlasterCharacter;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void ShowFramePackage(const FFramePackage& Package, const FColor& Color);

    /**
     * HitScan
     */
    UFUNCTION(Server, Reliable)
    void ServerScoreRequest(ABlasterCharacter* HitCharacter,  //
        const FVector_NetQuantize& TraceStart,                //
        const FVector_NetQuantize& HitLocation,               //
        float HitTime,
        float Damage,          //
        AWeapon* DamageCauser  //
    );

    /**
     * Projectile
     */

    UFUNCTION(Server, Reliable)
    void ProjectileServerScoreRequest(ABlasterCharacter* HitCharacter,  //
        const FVector_NetQuantize& TraceStart,                          //
        const FVector_NetQuantize100& InitialVelocity,                  //
        float HitTime,                                                  //
        float Damage,                                                   //
        AWeapon* DamageCauser                                           //
    );

    /**
     * Explosion projectile
     * Rocket
     * Grenade Launcher
     * Grenades
     */

    UFUNCTION(Server, Reliable)
    void ExplosionProjectileServerScoreRequest(TArray<ABlasterCharacter*>& HitCharacters,  //
        const FVector_NetQuantize& TraceStart,                                             //
        const FVector_NetQuantize100& InitialVelocity,                                     //
        float HitTime,                                                                     //
        float Damage                                                                       //
    );

    /**
     * Shotgun
     */
    UFUNCTION(Server, Reliable)
    void ShotgunServerScoreRequest(                       //
        const TArray<ABlasterCharacter*>& HitCharacters,  //
        const FVector_NetQuantize& TraceStart,            //
        const TArray<FVector_NetQuantize>& HitLocations,  //
        float HitTime,                                    //
        float Damage,                                     //
        AWeapon* DamageCauser                             //
    );

protected:
    virtual void BeginPlay() override;
    void SaveFramePackage(FFramePackage& Package);

    FFramePackage InterpBetweenFrames(      //
        const FFramePackage& OlderFrame,    //
        const FFramePackage& YoungerFrame,  //
        float HitTime);

    void CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);
    void MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);
    void ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);
    void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);
    void SaveFramePackage();
    FFramePackage GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime);

    /**
     * HitScan
     */
    FServerSideRewindResult ServerSideRewind(ABlasterCharacter* HitCharacter,  //
        const FVector_NetQuantize& TraceStart,                                 //
        const FVector_NetQuantize& HitLocation,                                //
        float HitTime);

    FServerSideRewindResult ConfirmHit(const FFramePackage& Package,  //
        ABlasterCharacter* HitCharacter,                              //
        const FVector_NetQuantize& TraceStart,                        //
        const FVector_NetQuantize& HitLocation);

    /**
     * Projectile
     */

    FServerSideRewindResult ProjectileServerSideRewind(ABlasterCharacter* HitCharacter,  //
        const FVector_NetQuantize& TraceStart,                                           //
        const FVector_NetQuantize100& InitialVelocity,                                   //
        float HitTime);

    FServerSideRewindResult ProjectileConfirmHit(const FFramePackage& Package,  //
        ABlasterCharacter* HitCharacter,                                        //
        const FVector_NetQuantize& TraceStart,                                  //
        const FVector_NetQuantize100& InitialVelocity,                          //
        float HitTime);

    /**
     * Shotgun
     */
    FShotgunServerSideRewindResult ShotgunServerSideRewind(  //
        const TArray<ABlasterCharacter*>& HitCharacters,     //
        const FVector_NetQuantize& TraceStart,               //
        const TArray<FVector_NetQuantize>& HitLocations,     //
        float HitTime);                                      //

    FShotgunServerSideRewindResult ShotgunConfirmHit(  //
        const TArray<FFramePackage>& FramePackages,    //
        const FVector_NetQuantize& TraceStart,         //
        const TArray<FVector_NetQuantize>& HitLocations);

private:
    bool IsCharacterValid();

    UPROPERTY()
    ABlasterCharacter* BlasterCharacter;

    UPROPERTY()
    ABlasterPlayerController* BlasterPlayerController;

    TDoubleLinkedList<FFramePackage> FrameHistory;

    UPROPERTY(EditAnywhere)
    float MaxRecordTime = 4.f;
};
