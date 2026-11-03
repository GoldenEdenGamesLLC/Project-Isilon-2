// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NativeGameplayTags.h"
#include "GameplayAbilitySystemLibrary.generated.h"


/**
 * 
 */
UCLASS()
class PROJECT_ISILON_2_API UGameplayAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure)
	static FGameplayTag GetRandomTagFromContainer(const FGameplayTagContainer& Container);

	UFUNCTION(BlueprintCallable)
	static FGameplayTagContainer GetAllChildrenTagsOfCategories(const FGameplayTagContainer& CategoryTags);

	UFUNCTION(BlueprintPure)
	static FGameplayTagContainer GetDamageTypeTags();

	/**
	 * Server only. Applies a damage effect from the attacker's ASC to the target's ASC.
	 * DamageByType maps a damage type tag (Combat.DamageType.*) to its base amount.
	 * Attacker ASC is looked up from DamageCauser, then from Instigator's pawn.
	 * DamageEffectClass defaults to UGE_Damage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat", meta = (AutoCreateRefTerm = "DamageByType"))
	static bool ApplyGASDamage(AActor* Target, AActor* DamageCauser, AController* Instigator, const TMap<FGameplayTag, float>& DamageByType, TSubclassOf<UGameplayEffect> DamageEffectClass = nullptr);
};
