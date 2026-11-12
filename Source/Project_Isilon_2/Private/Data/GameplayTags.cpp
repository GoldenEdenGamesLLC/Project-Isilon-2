#include "Data/GameplayTags.h"

namespace GameplayTags::Combat
{
    // Keep for a flat damage amount
    // UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Combat.Data.Damage", "Set By Caller Data Tag For Combat");

    namespace DamageTypes
    {
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(DamageTypeParent, "Combat.DamageType", "Primary Parent For Damage Types");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Physical, "Combat.DamageType.Physical", "Physical damage, reduced by armor");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Lightning, "Combat.DamageType.Lightning", "Lightning damage");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fire, "Combat.DamageType.Fire", "Fire damage");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Frost, "Combat.DamageType.Frost", "Frost damage");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poison, "Combat.DamageType.Poison", "Poison damage");
    }
}
