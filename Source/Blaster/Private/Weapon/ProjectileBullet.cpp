// Fill out your copyright notice in the Description page of Project Settings.

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "ProjectileBullet.h"

void AProjectileBullet::OnHit(
    UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter || !OwnerCharacter->GetController()) return;
    UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerCharacter->GetController(), this, UDamageType::StaticClass());
    Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
