#include "Data/GameplayTags.h"

namespace GameplayTags::Combat
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Combat.Data.Damage", "Set By Caller Data Tag For Combat");

    namespace DamageTypes
    {
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(DamageTypeParent, "Combat.DamageType", "Primary Parent For Damage Types");
    }
}