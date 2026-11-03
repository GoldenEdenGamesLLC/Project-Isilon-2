// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/HeroAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
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

	// Always start clean so a respawn re-runs init attributes (full health) and re-grants abilities
	ClearHeroData();

	for(const FHeroAbilityGrant& Grant : HeroData->Abilities)
	{
		if(!Grant.Ability)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(Grant.Ability, Grant.Level, INDEX_NONE, GetOwner());
		if(Grant.InputTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(Grant.InputTag);
		}

		GrantedAbilityHandles.Add(GiveAbility(Spec));
	}

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

	if(HeroData->HeroClassTag.IsValid())
	{
		GrantedHeroClassTag = HeroData->HeroClassTag;
		AddLooseGameplayTag(GrantedHeroClassTag, 1, EGameplayTagReplicationState::TagOnly);
	}
}

void UHeroAbilitySystemComponent::ClearHeroData()
{
	if(!IsOwnerActorAuthoritative())
	{
		return;
	}

	for(const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
	{
		ClearAbility(Handle);
	}
	GrantedAbilityHandles.Reset();

	for(const FActiveGameplayEffectHandle& Handle : GrantedEffectHandles)
	{
		RemoveActiveGameplayEffect(Handle);
	}
	GrantedEffectHandles.Reset();

	if(GrantedHeroClassTag.IsValid())
	{
		RemoveLooseGameplayTag(GrantedHeroClassTag, 1, EGameplayTagReplicationState::TagOnly);
		GrantedHeroClassTag = FGameplayTag();
	}
}

bool UHeroAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if(!InputTag.IsValid())
	{
		return false;
	}

	bool bFoundAbility = false;

	ABILITYLIST_SCOPE_LOCK();
	for(FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if(!Spec.Ability || !Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		bFoundAbility = true;
		AbilitySpecInputPressed(Spec);

		if(Spec.IsActive())
		{
			// Lets WaitInputPress tasks inside an active ability hear the press on the server
			if(const UGameplayAbility* Instance = Spec.GetPrimaryInstance())
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}
		}
		else
		{
			TryActivateAbility(Spec.Handle);
		}
	}

	return bFoundAbility;
}

void UHeroAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if(!InputTag.IsValid())
	{
		return;
	}

	ABILITYLIST_SCOPE_LOCK();
	for(FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if(!Spec.Ability || !Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		AbilitySpecInputReleased(Spec);

		if(Spec.IsActive())
		{
			if(const UGameplayAbility* Instance = Spec.GetPrimaryInstance())
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}
		}
	}
}
