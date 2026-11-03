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
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fire);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frost);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Poison);
    }
}

namespace GameplayTags::Heroes
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Barbarian);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ranger);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Wizard);
}

namespace GameplayTags::Input
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(BasicAttack);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability1);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability2);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ultimate);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility);
}
