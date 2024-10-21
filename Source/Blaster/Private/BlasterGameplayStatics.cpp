#include "GameFramework/DamageType.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "BlasterCharacter.h"
#include "Engine/DamageEvents.h"
#include "BlasterGameplayStatics.h"

void UBlasterGameplayStatics::GetOverlapActorsBySphereTrace(UObject* WorldObject,  //
    TMap<AActor*, FHitResult>& OutOverlapCharacters,                               //
    FVector Origin,                                                                //
    FName Tag,                                                                     //
    ECollisionChannel CollisionChannel,                                            //
    float Radius)
{
    if (!WorldObject) return;

    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(CollisionChannel);

    WorldObject->GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius));

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* OverlapActor = Overlap.OverlapObjectHandle.FetchActor();
        if (OverlapActor && OverlapActor->ActorHasTag(Tag) && OverlapActor->CanBeDamaged() && Overlap.Component.IsValid())
        {
            if (!OutOverlapCharacters.Contains(OverlapActor))
            {
                FVector const FakeHitLoc = Overlap.Component->GetComponentLocation();
                FVector const FakeHitNorm = (Origin - FakeHitLoc).GetSafeNormal();
                FHitResult Hit = FHitResult(Overlap.Component.Get()->GetOwner(), Overlap.Component.Get(), FakeHitLoc, FakeHitNorm);
                OutOverlapCharacters.Add(OverlapActor, Hit);
            }
        }
    }
}

void UBlasterGameplayStatics::GetOverlapCharactersBySphereTrace(UObject* WorldObject,  //
    TMap<ABlasterCharacter*, FHitResult>& OutOverlapCharacters,                        //
    FVector Origin,                                                                    //
    ECollisionChannel CollisionChannel,                                                //
    float Radius)
{
    if (!WorldObject) return;

    TMap<AActor*, FHitResult> OverlapCharacters;
    GetOverlapActorsBySphereTrace(WorldObject, OverlapCharacters, Origin, FName("BlasterCharacter"), CollisionChannel, Radius);
    for (auto& Overlap : OverlapCharacters)
    {
        if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(Overlap.Key))
        {
            OutOverlapCharacters.Add(HitCharacter, Overlap.Value);
        }
    }
}

void UBlasterGameplayStatics::GetOverlapCharactersBySphereTrace(UObject* WorldObject,  //
    TArray<ABlasterCharacter*>& OutOverlapActors,                                      //
    FVector Origin,                                                                    //
    ECollisionChannel CollisionChannel,                                                //
    float Radius)
{
    if (!WorldObject) return;

    TMap<AActor*, FHitResult> OverlapCharacters;
    GetOverlapActorsBySphereTrace(WorldObject, OverlapCharacters, Origin, FName("BlasterCharacter"), CollisionChannel, Radius);
    for (auto& Overlap : OverlapCharacters)
    {
        if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(Overlap.Key))
        {
            OutOverlapActors.AddUnique(HitCharacter);
        }
    }
}

void UBlasterGameplayStatics::MakeRadialDamageWithFallOff(const TMap<AActor*, FHitResult>& OverlapActorsMap,  //
    const TArray<AActor*>& MustIncludeActors,                                                                 //
    const FVector& Origin,                                                                                    //
    float Damage,                                                                                             //
    float DamageInnerRadius,                                                                                  //
    float DamageOuterRadius,                                                                                  //
    AController* InstigatorController,                                                                        //
    AActor* DamageCauser)
{

    if (OverlapActorsMap.Num() > 0)
    {
        FRadialDamageEvent DmgEvent;
        DmgEvent.DamageTypeClass = UDamageType::StaticClass();
        DmgEvent.Origin = Origin;
        DmgEvent.Params = FRadialDamageParams(Damage, 10.f, DamageInnerRadius, DamageOuterRadius, 1.f);

        for (auto& OverlapActorsPair : OverlapActorsMap)
        {
            if (OverlapActorsPair.Key && MustIncludeActors.Contains(OverlapActorsPair.Key))
            {
                AActor* ConfirmOverlapActor = OverlapActorsPair.Key;
                DmgEvent.ComponentHits = TArray{OverlapActorsPair.Value};

                ConfirmOverlapActor->TakeDamage(  //
                    Damage,                       //
                    DmgEvent,                     //
                    InstigatorController,         //
                    DamageCauser);
            }
        }
    }
}

void UBlasterGameplayStatics::MakeRadialDamageWithFallOff(const TMap<AActor*, FHitResult>& OverlapActorsMap,  //
    const FVector& Origin,                                                                                    //
    float Damage,                                                                                             //
    float DamageInnerRadius,                                                                                  //
    float DamageOuterRadius,                                                                                  //
    AController* InstigatorController,                                                                        //
    AActor* DamageCauser)
{

    if (OverlapActorsMap.Num() > 0)
    {
        FRadialDamageEvent DmgEvent;
        DmgEvent.DamageTypeClass = UDamageType::StaticClass();
        DmgEvent.Origin = Origin;
        DmgEvent.Params = FRadialDamageParams(Damage, 10.f, DamageInnerRadius, DamageOuterRadius, 1.f);

        for (auto& OverlapActorsPair : OverlapActorsMap)
        {
            if (OverlapActorsPair.Key)
            {
                AActor* ConfirmOverlapActor = OverlapActorsPair.Key;
                DmgEvent.ComponentHits = TArray{OverlapActorsPair.Value};

                ConfirmOverlapActor->TakeDamage(  //
                    Damage,                       //
                    DmgEvent,                     //
                    InstigatorController,         //
                    DamageCauser);
            }
        }
    }
}
