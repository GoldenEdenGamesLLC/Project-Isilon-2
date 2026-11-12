// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "HeroAbilitySystemComponent.generated.h"

class UHeroData;

/**
 * ASC for player heroes. Lives on AHeroPlayerState so it survives pawn death/respawn.
 * Holds the hero's stats (UTagAttributeSet) and stat effects; abilities stay in the hero classes.
 */
UCLASS()
class PROJECT_ISILON_2_API UHeroAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UHeroAbilitySystemComponent();

	// Server only. Removes the previous hero's effects, then applies HeroData's init and startup effects.
	void ApplyHeroData(const UHeroData* HeroData);

	// Server only. Removes everything applied by ApplyHeroData.
	void ClearHeroData();

private:
	TArray<FActiveGameplayEffectHandle> GrantedEffectHandles;
};
