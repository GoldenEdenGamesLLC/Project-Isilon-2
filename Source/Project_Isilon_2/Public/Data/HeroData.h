// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HeroData.generated.h"

class UGameplayEffect;

/**
 * A hero's starting stats.
 * Make one asset per hero (DA_Hero_Barbarian, DA_Hero_Ranger, DA_Hero_Wizard, ...)
 * and assign it on that hero's Blueprint.
 */
UCLASS(BlueprintType)
class PROJECT_ISILON_2_API UHeroData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Instant effect that sets this hero's base attributes (MaxHealth, Armor, SpellPower, ...)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Attributes")
	TSubclassOf<UGameplayEffect> InitAttributesEffect;

	// Extra effects applied on spawn (regen, passive buffs, ...)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Attributes")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;
};
