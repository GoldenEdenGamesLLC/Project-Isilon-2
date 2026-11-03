// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/DamageExecCalc.h"

#include "AbilitySystem/TagAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Data/GameplayTags.h"

namespace
{
	struct FDamageStatics
	{
		// Source (snapshot = false, read at the moment the hit lands)
		FGameplayEffectAttributeCaptureDefinition AttackPowerDef;
		FGameplayEffectAttributeCaptureDefinition SpellPowerDef;
		FGameplayEffectAttributeCaptureDefinition CritChanceDef;
		FGameplayEffectAttributeCaptureDefinition CritMultiplierDef;

		// Target
		FGameplayEffectAttributeCaptureDefinition ArmorDef;
		FGameplayEffectAttributeCaptureDefinition FireResistanceDef;
		FGameplayEffectAttributeCaptureDefinition FrostResistanceDef;
		FGameplayEffectAttributeCaptureDefinition PoisonResistanceDef;

		// Elemental damage type tag -> target resistance it is reduced by
		TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition> ElementalResistances;

		FDamageStatics()
		{
			AttackPowerDef = FGameplayEffectAttributeCaptureDefinition(UTagAttributeSet::GetAttackPowerAttribute(), EGameplayEffectAttributeCaptureSource::Source, false);
			SpellPowerDef = FGameplayEffectAttributeCaptureDefinition(UTagAttributeSet::GetSpellPowerAttribute(), EGameplayEffectAttributeCaptureSource::Source, false);
			CritChanceDef = FGameplayEffectAttributeCaptureDefinition(UTagAttributeSet::GetCritChanceAttribute(), EGameplayEffectAttributeCaptureSource::Source, false);
			CritMultiplierDef = FGameplayEffectAttributeCaptureDefinition(UTagAttributeSet::GetCritMultiplierAttribute(), EGameplayEffectAttributeCaptureSource::Source, false);

			ArmorDef = FGameplayEffectAttributeCaptureDefinition(UTagAttributeSet::GetArmorAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
			FireResistanceDef = FGameplayEffectAttributeCaptureDefinition(UTagAttributeSet::GetFireResistanceAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
			FrostResistanceDef = FGameplayEffectAttributeCaptureDefinition(UTagAttributeSet::GetFrostResistanceAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
			PoisonResistanceDef = FGameplayEffectAttributeCaptureDefinition(UTagAttributeSet::GetPoisonResistanceAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);

			using namespace GameplayTags::Combat::DamageTypes;
			ElementalResistances.Add(Fire, FireResistanceDef);
			ElementalResistances.Add(Frost, FrostResistanceDef);
			ElementalResistances.Add(Poison, PoisonResistanceDef);
		}
	};

	const FDamageStatics& DamageStatics()
	{
		static FDamageStatics Statics;
		return Statics;
	}

	float GetCapturedValue(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FGameplayEffectAttributeCaptureDefinition& Def, const FAggregatorEvaluateParameters& EvalParams)
	{
		float Value = 0.0f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Def, EvalParams, Value);
		return Value;
	}
}

UDamageExecCalc::UDamageExecCalc()
{
	const FDamageStatics& Statics = DamageStatics();

	RelevantAttributesToCapture.Add(Statics.AttackPowerDef);
	RelevantAttributesToCapture.Add(Statics.SpellPowerDef);
	RelevantAttributesToCapture.Add(Statics.CritChanceDef);
	RelevantAttributesToCapture.Add(Statics.CritMultiplierDef);
	RelevantAttributesToCapture.Add(Statics.ArmorDef);
	RelevantAttributesToCapture.Add(Statics.FireResistanceDef);
	RelevantAttributesToCapture.Add(Statics.FrostResistanceDef);
	RelevantAttributesToCapture.Add(Statics.PoisonResistanceDef);
}

void UDamageExecCalc::Execute_Implementation(const FGameplayEffectCustomExecutionParameters &ExecutionParams, FGameplayEffectCustomExecutionOutput &OutExecutionOutput) const
{
	const FDamageStatics& Statics = DamageStatics();
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	const float AttackPower = FMath::Max(GetCapturedValue(ExecutionParams, Statics.AttackPowerDef, EvalParams), 0.0f);
	const float SpellPower = FMath::Max(GetCapturedValue(ExecutionParams, Statics.SpellPowerDef, EvalParams), 0.0f);

	float TotalDamage = 0.0f;

	// Physical
	const float PhysicalBase = Spec.GetSetByCallerMagnitude(GameplayTags::Combat::DamageTypes::Physical, false, 0.0f);
	if(PhysicalBase > 0.0f)
	{
		const float Armor = FMath::Max(GetCapturedValue(ExecutionParams, Statics.ArmorDef, EvalParams), 0.0f);
		const float ArmorReduction = Armor / (Armor + FMath::Max(ArmorConstant, 1.0f));

		TotalDamage += PhysicalBase * (1.0f + AttackPower / 100.0f) * (1.0f - ArmorReduction);
	}

	// Elemental
	for(const TPair<FGameplayTag, FGameplayEffectAttributeCaptureDefinition>& Pair : Statics.ElementalResistances)
	{
		const float ElementalBase = Spec.GetSetByCallerMagnitude(Pair.Key, false, 0.0f);
		if(ElementalBase <= 0.0f)
		{
			continue;
		}

		const float Resistance = FMath::Clamp(GetCapturedValue(ExecutionParams, Pair.Value, EvalParams), 0.0f, 1.0f);

		TotalDamage += ElementalBase * (1.0f + SpellPower / 100.0f) * (1.0f - Resistance);
	}

	if(TotalDamage <= 0.0f)
	{
		return;
	}

	// Crit
	const float CritChance = FMath::Clamp(GetCapturedValue(ExecutionParams, Statics.CritChanceDef, EvalParams), 0.0f, 1.0f);
	if(CritChance > 0.0f && FMath::FRand() < CritChance)
	{
		const float CritMultiplier = FMath::Max(GetCapturedValue(ExecutionParams, Statics.CritMultiplierDef, EvalParams), 1.0f);
		TotalDamage *= CritMultiplier;
		// TODO: add a crit tag to the effect context so UI / cues can show crit numbers
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UTagAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, TotalDamage));
}
