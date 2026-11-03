// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/EnemyCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/TagAttributeSet.h"

AEnemyCharacterBase::AEnemyCharacterBase()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UTagAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AEnemyCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if(HasAuthority())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UTagAttributeSet::GetHealthAttribute()).AddUObject(this, &AEnemyCharacterBase::OnHealthAttributeChanged);
	}
}

void AEnemyCharacterBase::InitHealthAttributes(float InMaxHealth)
{
	if(!HasAuthority())
	{
		return;
	}

	// MaxHealth first so Health isn't clamped to the old max
	AbilitySystemComponent->SetNumericAttributeBase(UTagAttributeSet::GetMaxHealthAttribute(), InMaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(UTagAttributeSet::GetHealthAttribute(), InMaxHealth);
}

void AEnemyCharacterBase::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	HandleHealthChanged(Data.OldValue, Data.NewValue);
}
