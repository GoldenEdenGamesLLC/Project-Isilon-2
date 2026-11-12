#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace GameplayTags::Combat
{
    // Keep for a flat damage amount
    // UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);

    namespace DamageTypes
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageTypeParent);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Physical);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lightning);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fire);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frost);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Poison);
    }
}
