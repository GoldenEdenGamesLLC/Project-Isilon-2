// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "HeroAbilitySystemComponent.generated.h"

class UHeroData;

/**
 * ASC for player heroes. Lives on AHeroPlayerState so it survives pawn death/respawn.
 * Each hero's abilities and attributes come from its UHeroData asset; switching hero
 * clears the previous grants and applies the new ones.
 */
UCLASS()
class PROJECT_ISILON_2_API UHeroAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UHeroAbilitySystemComponent();

	// Server only. Removes any previous hero's grants, then grants HeroData's abilities and effects.
	void ApplyHeroData(const UHeroData* HeroData);

	// Server only. Removes everything granted by ApplyHeroData.
	void ClearHeroData();

	// Call from Enhanced Input bindings. Activates / forwards input to abilities granted with this InputTag.
	// Returns false if no granted ability uses this InputTag, so the caller can fall back to old code.
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool AbilityInputTagPressed(const FGameplayTag& InputTag);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	bool HasAppliedHeroData() const { return AppliedHeroData != nullptr; }

private:
	UPROPERTY()
	TObjectPtr<const UHeroData> AppliedHeroData;

	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
	TArray<FActiveGameplayEffectHandle> GrantedEffectHandles;
	FGameplayTag GrantedHeroClassTag;
};
