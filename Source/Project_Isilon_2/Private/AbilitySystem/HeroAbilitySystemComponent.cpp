// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/HeroAbilitySystemComponent.h"

#include "Data/HeroData.h"

UHeroAbilitySystemComponent::UHeroAbilitySystemComponent()
{
	SetIsReplicated(true);
	// Owning client gets full effect info, everyone else only gets tags/cues
	SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void UHeroAbilitySystemComponent::ApplyHeroData(const UHeroData* HeroData)
{
	if(!IsOwnerActorAuthoritative() || !HeroData)
	{
		return;
	}

	// Always start clean so a respawn re-runs init attributes (full health)
	ClearHeroData();

	FGameplayEffectContextHandle Context = MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	auto ApplyEffect = [this, &Context](const TSubclassOf<UGameplayEffect>& EffectClass)
	{
		if(!EffectClass)
		{
			return;
		}

		const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(EffectClass, 1.0f, Context);
		if(SpecHandle.IsValid())
		{
			const FActiveGameplayEffectHandle Handle = ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			// Instant effects (like init attributes) return an invalid handle; only track the ones we can remove
			if(Handle.IsValid())
			{
				GrantedEffectHandles.Add(Handle);
			}
		}
	};

	ApplyEffect(HeroData->InitAttributesEffect);
	for(const TSubclassOf<UGameplayEffect>& EffectClass : HeroData->StartupEffects)
	{
		ApplyEffect(EffectClass);
	}
}

void UHeroAbilitySystemComponent::ClearHeroData()
{
	if(!IsOwnerActorAuthoritative())
	{
		return;
	}

	for(const FActiveGameplayEffectHandle& Handle : GrantedEffectHandles)
	{
		RemoveActiveGameplayEffect(Handle);
	}
	GrantedEffectHandles.Reset();
}
