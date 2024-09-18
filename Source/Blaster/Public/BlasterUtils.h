#pragma once

#include "GameFramework/Controller.h"
#include "GameFramework/HUD.h"

class AActor;
class ACharacter;

class BlasterUtils
{
public:
    template <typename T>
    static bool CastOrUseExistsActor(T*& TargetActor, AActor* CastActor)
    {
        TargetActor = TargetActor == nullptr ? Cast<T>(CastActor) : TargetActor;
        return TargetActor != nullptr;
    }

    template <typename T1, typename T2>
    static typename TEnableIf<TIsDerivedFrom<T1, ACharacter>::Value && TIsDerivedFrom<T2, AController>::Value, bool>::Type
    CastOrUseExistsActors(T1*& TargetActor1, T2*& TargetActor2, AActor* CastActor)
    {
        if (CastOrUseExistsActor<T1>(TargetActor1, CastActor) && CastOrUseExistsActor<T2>(TargetActor2, TargetActor1->GetController()))
        {
            return true;
        }
        return false;
    }

    template <typename T1, typename T2>
    static typename TEnableIf<TIsDerivedFrom<T1, AController>::Value && TIsDerivedFrom<T2, AHUD>::Value, bool>::Type CastOrUseExistsActors(
        T1*& TargetActor1, T2*& TargetActor2, AActor* CastActor)
    {
        if (CastOrUseExistsActor<T1>(TargetActor1, CastActor) && CastOrUseExistsActor<T2>(TargetActor2, TargetActor1->GetHUD()))
        {
            return true;
        }
        return false;
    }
};