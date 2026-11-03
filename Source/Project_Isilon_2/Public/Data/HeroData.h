// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "HeroData.generated.h"

class UGameplayAbility;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FHeroAbilityGrant
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayAbility> Ability;

	// Input tag that activates this ability (e.g. Input.Ability.Primary). Leave empty for passives.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Input"))
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Level = 1;
};

/**
 * Everything that makes one hero different on the GAS side.
 * Make one asset per hero (DA_Hero_Barbarian, DA_Hero_Ranger, DA_Hero_Wizard, ...)
 * and assign it on that hero's Blueprint.
 */
UCLASS(BlueprintType)
class PROJECT_ISILON_2_API UHeroData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Identifies the hero class, e.g. Hero.Barbarian. Added as a loose tag on the ASC.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero", meta = (Categories = "Hero"))
	FGameplayTag HeroClassTag;

	// Instant effect that sets this hero's base attributes (MaxHealth, Armor, SpellPower, ...)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Attributes")
	TSubclassOf<UGameplayEffect> InitAttributesEffect;

	// Extra effects applied on spawn (regen, passive buffs, ...)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Attributes")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Abilities")
	TArray<FHeroAbilityGrant> Abilities;
};
