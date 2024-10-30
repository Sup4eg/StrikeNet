#pragma once

#include "CarryItemTypes.generated.h"

#define TRACE_LENGTH 80000
#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252

UENUM(BlueprintType)
enum class ECarryItemState : uint8
{
    ECIS_Initial UMETA(DisplayName = "Initial State"),
    ECIS_Equipped UMETA(DisplayName = "Equiped"),
    ECIS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
    ECIS_Dropped UMETA(DisplayName = "Dropped"),
    ECIS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
    EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
    EWT_Pistol UMETA(DisplayName = "Pistol"),
    EWT_SMG UMETA(DisplayName = "SMG"),
    EWT_Shotgun UMETA(DisplayName = "Shotgun"),
    EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),
    EWT_GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),
    EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EProjectileType : uint8
{
    EPT_ProjectileBullet UMETA(DisplayName = "Projectile Bullet"),
    EPT_ProjectileRocket UMETA(DisplayName = "Projectile Rocket"),
    EPT_ProjectileGrenade UMETA(DisplayName = "Projectile Grenade"),
    EPT_ProjectileThrowGrenade UMETA(DisplayName = "Projectile Throw Grenade"),
    EPT_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
    EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
    EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
    EFT_Shotgun UMETA(DispalyName = "Shotgun Weapon"),
    EFT_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FDecalData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditDefaultsOnly)
    UMaterialInterface* Material;

    UPROPERTY(EditDefaultsOnly)
    FVector Size = FVector(10.0f);

    UPROPERTY(EditDefaultsOnly)
    float LifeTime = 60.f;

    UPROPERTY(EditDefaultsOnly)
    float FadeOutTime = 0.7f;
};

USTRUCT(BlueprintType)
struct FImpactData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere)
    UParticleSystem* ImpactParticles;

    UPROPERTY(EditAnywhere)
    USoundBase* ImpactSound;

    UPROPERTY(EditDefaultsOnly, BluePrintReadWrite, Category = "VFX")
    FDecalData DecalData;
};