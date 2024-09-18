// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;

UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    /**
     * Replication notifies
     */
    virtual void OnRep_Score() override;

    UFUNCTION()
    virtual void OnRep_Defeats();

    UFUNCTION()
    virtual void OnRep_KilledBy();

    void AddToScore(float ScoreAmount);
    void AddToDefeats(float DefeatsAmount);
    void AddKilledBy(const FName& NewKilledBy);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void HandleScore();
    void HandleDefeats();
    void HandleKilledBy();
    bool IsControllerValid();

    UPROPERTY()
    ABlasterCharacter* Character;

    UPROPERTY()
    ABlasterPlayerController* Controller;

    UPROPERTY(ReplicatedUsing = OnRep_Defeats)
    int32 Defeats;

    UPROPERTY(ReplicatedUsing = OnRep_KilledBy)
    FName KilledBy = FName();

public:
    FORCEINLINE void SetDefeats(int32 NewDefeats) { Defeats = NewDefeats; };
    FORCEINLINE int32 GetDefeats() const { return Defeats; };
    FORCEINLINE FName GetKilledBy() const { return KilledBy; };
    FORCEINLINE void SetKilledBy(const FName& NewKilledBy) { KilledBy = NewKilledBy; };
};