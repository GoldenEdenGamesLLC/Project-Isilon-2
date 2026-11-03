// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GE_Damage.h"

#include "AbilitySystem/DamageExecCalc.h"

UGE_Damage::UGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition Execution;
	Execution.CalculationClass = UDamageExecCalc::StaticClass();
	Executions.Add(Execution);
}
