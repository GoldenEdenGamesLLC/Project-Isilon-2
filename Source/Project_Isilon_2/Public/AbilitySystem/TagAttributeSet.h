// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "TagAttributeSet.generated.h"

// Generates GetX(), SetX(), InitX() and GetXAttribute() for an attribute
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Shared attribute set for every hero (and enemies, if they get an ASC).
 * Per-hero values (Barbarian = high Health/Armor, Wizard = high Mana/SpellPower, etc.)
 * are set with an "init attributes" GameplayEffect in each hero's UHeroData asset.
 */
UCLASS()
class PROJECT_ISILON_2_API UTagAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UTagAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// ===========================================
	// Vitals
	// ===========================================
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Vitals")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Vitals")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, MaxHealth)

	//==========================================
	// Offense
	// ===========================================
	// Scales Physical damage
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "Offense")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, AttackPower)

	// Scales Fire / Frost / Poison damage
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpellPower, Category = "Offense")
	FGameplayAttributeData SpellPower;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, SpellPower)

	// 0..1
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritChance, Category = "Offense")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, CritChance)

	// e.g. 1.5 = +50% damage on crit
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritMultiplier, Category = "Offense")
	FGameplayAttributeData CritMultiplier;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, CritMultiplier)

	// ===========================================
	// Defense
	// ===========================================
	// Reduces Physical damage with diminishing returns: Armor / (Armor + ArmorConstant)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "Defense")
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, Armor)

	// Resistances are 0..1 (0.25 = 25% less damage of that type)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_LightningResistance, Category = "Defense")
	FGameplayAttributeData LightningResistance;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, LightningResistance)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FireResistance, Category = "Defense")
	FGameplayAttributeData FireResistance;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, FireResistance)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FrostResistance, Category = "Defense")
	FGameplayAttributeData FrostResistance;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, FrostResistance)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PoisonResistance, Category = "Defense")
	FGameplayAttributeData PoisonResistance;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, PoisonResistance)

	// ===========================================
	// Meta (server only, not replicated)
	// ===========================================
	// Written by UDamageExecCalc, converted into Health loss in PostGameplayEffectExecute
	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UTagAttributeSet, IncomingDamage)

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SpellPower(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CritChance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CritMultiplier(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_LightningResistance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_FireResistance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_FrostResistance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PoisonResistance(const FGameplayAttributeData& OldValue);
	
};
