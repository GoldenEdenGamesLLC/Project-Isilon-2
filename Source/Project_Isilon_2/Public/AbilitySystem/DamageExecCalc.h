// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "DamageExecCalc.generated.h"

/**
 * Damage pipeline for an instant "GE_Damage" effect.
 *
 * The effect carries one SetByCaller magnitude per damage type, keyed by the
 * damage type tag itself (Combat.DamageType.Physical, .Fire, .Frost, .Poison).
 * A single hit can mix types, e.g. Physical 40 + Fire 15.
 *
 * Per type:  Base * (1 + SourcePower / 100) * (1 - TargetMitigation)
 *   Physical uses AttackPower and Armor / (Armor + ArmorConstant)
 *   Elemental uses SpellPower and the matching resistance
 * Then a single crit roll on the total, and the result goes to IncomingDamage.
 */
UCLASS()
class PROJECT_ISILON_2_API UDamageExecCalc : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UDamageExecCalc();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	// Higher = armor is less effective. At Armor == ArmorConstant, physical damage is halved.
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float ArmorConstant = 100.0f;
};
