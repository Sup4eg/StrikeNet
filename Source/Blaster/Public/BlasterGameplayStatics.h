#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/HitResult.h"
#include "Engine/EngineTypes.h"
#include "BlasterGameplayStatics.generated.h"

class ABlasterCharacter;

UCLASS()
class UBlasterGameplayStatics : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    static void GetOverlapActorsBySphereTrace(UObject* WorldObject, TMap<AActor*, FHitResult>& OutOverlapActors,  //
        FVector Origin,                                                                                           //
        FName Tag,                                                                                                //
        ECollisionChannel CollisionChannel,                                                                       //
        float Radius);

    static void GetOverlapCharactersBySphereTrace(UObject* WorldObject, TMap<ABlasterCharacter*, FHitResult>& OutOverlapActors,  //
        FVector Origin,                                                                                                          //
        ECollisionChannel CollisionChannel,                                                                                      //
        float Radius);

    static void GetOverlapCharactersBySphereTrace(UObject* WorldObject, TArray<ABlasterCharacter*>& OutOverlapActors,  //
        FVector Origin,                                                                                                //
        ECollisionChannel CollisionChannel,                                                                            //
        float Radius);

    static void MakeRadialDamageWithFallOff(const TMap<AActor*, FHitResult>& OverlapActorsMap,  //
        const TArray<AActor*>& MustIncludeActors,                                               //
        const FVector& Origin,                                                                  //
        float Damage,                                                                           //
        float DamageInnerRadius,                                                                //
        float DamageOuterRadius,
        AController* InstigatorController,  //
        AActor* DamageCauser                //
    );

    static void MakeRadialDamageWithFallOff(const TMap<AActor*, FHitResult>& OverlapActorsMap,  //
        const FVector& Origin,                                                                  //
        float Damage,                                                                           //
        float DamageInnerRadius,                                                                //
        float DamageOuterRadius,
        AController* InstigatorController,  //
        AActor* DamageCauser                //
    );
};