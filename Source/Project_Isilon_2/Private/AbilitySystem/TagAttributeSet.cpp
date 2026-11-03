// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/TagAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UTagAttributeSet::UTagAttributeSet()
{
	// Safe defaults; real values come from each hero's init attributes effect
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitResource(100.0f);
	InitMaxResource(100.0f);
	InitAttackPower(0.0f);
	InitSpellPower(0.0f);
	InitCritChance(0.0f);
	InitCritMultiplier(1.5f);
	InitArmor(0.0f);
	InitFireResistance(0.0f);
	InitFrostResistance(0.0f);
	InitPoisonResistance(0.0f);
	InitIncomingDamage(0.0f);
}

void UTagAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, Resource, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, MaxResource, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, SpellPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, CritMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, FireResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, FrostResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTagAttributeSet, PoisonResistance, COND_None, REPNOTIFY_Always);
}

void UTagAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Only clamps the displayed value; the final clamp on Health happens in PostGameplayEffectExecute
	if(Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if(Attribute == GetResourceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxResource());
	}
	else if(Attribute == GetMaxHealthAttribute() || Attribute == GetMaxResourceAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if(Attribute == GetCritChanceAttribute()
		|| Attribute == GetFireResistanceAttribute()
		|| Attribute == GetFrostResistanceAttribute()
		|| Attribute == GetPoisonResistanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
	}
}

void UTagAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if(Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalDamage = GetIncomingDamage();
		SetIncomingDamage(0.0f);

		if(LocalDamage > 0.0f)
		{
			const float NewHealth = FMath::Clamp(GetHealth() - LocalDamage, 0.0f, GetMaxHealth());
			SetHealth(NewHealth);

			if(NewHealth <= 0.0f)
			{
				// TODO: send a death gameplay event / apply a "State.Dead" tag here
				UE_LOG(LogTemp, Warning, TEXT("[GAS] %s died"), *GetNameSafe(GetOwningActor()));
			}
		}
	}
	else if(Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if(Data.EvaluatedData.Attribute == GetResourceAttribute())
	{
		SetResource(FMath::Clamp(GetResource(), 0.0f, GetMaxResource()));
	}
}

void UTagAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, Health, OldValue);
}

void UTagAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, MaxHealth, OldValue);
}

void UTagAttributeSet::OnRep_Resource(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, Resource, OldValue);
}

void UTagAttributeSet::OnRep_MaxResource(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, MaxResource, OldValue);
}

void UTagAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, AttackPower, OldValue);
}

void UTagAttributeSet::OnRep_SpellPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, SpellPower, OldValue);
}

void UTagAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, CritChance, OldValue);
}

void UTagAttributeSet::OnRep_CritMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, CritMultiplier, OldValue);
}

void UTagAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, Armor, OldValue);
}

void UTagAttributeSet::OnRep_FireResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, FireResistance, OldValue);
}

void UTagAttributeSet::OnRep_FrostResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, FrostResistance, OldValue);
}

void UTagAttributeSet::OnRep_PoisonResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTagAttributeSet, PoisonResistance, OldValue);
}
