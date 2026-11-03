// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/HeroPlayerState.h"

#include "AbilitySystem/HeroAbilitySystemComponent.h"
#include "AbilitySystem/TagAttributeSet.h"

AHeroPlayerState::AHeroPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UHeroAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// Subobject of the owner of the ASC is registered with it automatically
	AttributeSet = CreateDefaultSubobject<UTagAttributeSet>(TEXT("AttributeSet"));

	// PlayerState defaults to a low update rate, which makes GAS feel laggy
	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* AHeroPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
