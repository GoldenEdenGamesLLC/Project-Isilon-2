#include "Data/GameplayTags.h"

namespace GameplayTags::Combat
{
    // Keep for a flat damage amount
    // UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Combat.Data.Damage", "Set By Caller Data Tag For Combat");

    namespace DamageTypes
    {
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(DamageTypeParent, "Combat.DamageType", "Primary Parent For Damage Types");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Physical, "Combat.DamageType.Physical", "Physical damage, reduced by armor");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fire, "Combat.DamageType.Fire", "Fire damage");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Frost, "Combat.DamageType.Frost", "Frost damage");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poison, "Combat.DamageType.Poison", "Poison damage");
    }
}

namespace GameplayTags::Heroes
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Barbarian, "Hero.Barbarian", "Barbarian hero class");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ranger, "Hero.Ranger", "Ranger hero class");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Wizard, "Hero.Wizard", "Wizard hero class");
}

namespace GameplayTags::Input
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(BasicAttack, "Input.BasicAttack", "Activates the hero's basic attack");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability1, "Input.Ability1", "Activates the hero's first ability");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability2, "Input.Ability2", "Activates the hero's second ability");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Utility, "Input.Utility", "Activates the hero's utility/dash ability");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ultimate, "Input.Ultimate", "Activates the hero's ultimate");
    // may add this for the future anyways
    // UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dash, "Input.Dash", "Activates the hero's dash");
}
