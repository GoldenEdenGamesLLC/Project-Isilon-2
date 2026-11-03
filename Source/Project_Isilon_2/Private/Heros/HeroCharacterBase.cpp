// Fill out your copyright notice in the Description page of Project Settings.


#include "Heros/HeroCharacterBase.h"

#include "AbilitySystem/HeroAbilitySystemComponent.h"
#include "AbilitySystem/TagAttributeSet.h"
#include "Data/GameplayTags.h"
#include "Data/HeroData.h"
#include "Game/HeroPlayerState.h"
#include "Libraries/GameplayAbilitySystemLibrary.h"

#include "EnhancedInputComponent.h"
#include "InputActionValue.h"

AHeroCharacterBase::AHeroCharacterBase()
{
	
}

UAbilitySystemComponent* AHeroCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent.Get();
}

void AHeroCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitAbilityActorInfo();

	UHeroAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if(!ASC)
	{
		return;
	}

	ASC->ApplyHeroData(HeroData);

	// No init effect yet: start from the hero's own C++ health values
	if(!HeroData || !HeroData->InitAttributesEffect)
	{
		float StartHealth = 100.0f;
		float StartMaxHealth = 100.0f;
		GetDefaultHealth(StartHealth, StartMaxHealth);

		// MaxHealth first so Health isn't clamped to the old max
		ASC->SetNumericAttributeBase(UTagAttributeSet::GetMaxHealthAttribute(), StartMaxHealth);
		ASC->SetNumericAttributeBase(UTagAttributeSet::GetHealthAttribute(), FMath::Min(StartHealth, StartMaxHealth));
	}
}

void AHeroCharacterBase::UnPossessed()
{
	UnbindHealthChanged();

	Super::UnPossessed();
}

void AHeroCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilityActorInfo();
}

void AHeroCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindHealthChanged();

	Super::EndPlay(EndPlayReason);
}

void AHeroCharacterBase::InitAbilityActorInfo()
{
	AHeroPlayerState* HeroPlayerState = GetPlayerState<AHeroPlayerState>();
	if(!HeroPlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GAS] %s has no AHeroPlayerState. Set PlayerStateClass on the game mode."), *GetName());
		return;
	}

	UHeroAbilitySystemComponent* ASC = HeroPlayerState->GetHeroAbilitySystemComponent();
	ASC->InitAbilityActorInfo(HeroPlayerState, this);

	UnbindHealthChanged();

	AbilitySystemComponent = ASC;
	AttributeSet = HeroPlayerState->GetAttributeSet();

	if(HasAuthority())
	{
		HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UTagAttributeSet::GetHealthAttribute()).AddUObject(this, &AHeroCharacterBase::OnHealthAttributeChanged);
	}
}

void AHeroCharacterBase::UnbindHealthChanged()
{
	if(UHeroAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		if(HealthChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UTagAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		}
	}

	HealthChangedHandle.Reset();
}

void AHeroCharacterBase::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	HandleHealthChanged(Data.OldValue, Data.NewValue);
}

float AHeroCharacterBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if(HasAuthority() && ActualDamage > 0.0f)
	{
		TMap<FGameplayTag, float> DamageByType;
		DamageByType.Add(GameplayTags::Combat::DamageTypes::Physical, ActualDamage);
		UGameplayAbilitySystemLibrary::ApplyGASDamage(this, DamageCauser, EventInstigator, DamageByType);
	}

	return ActualDamage;
}

void AHeroCharacterBase::GetDefaultHealth(float& OutHealth, float& OutMaxHealth) const
{
	OutHealth = 100.0f;
	OutMaxHealth = 100.0f;
}

void AHeroCharacterBase::BindAbilityInput(UEnhancedInputComponent* EnhancedInputComponent, const UInputAction* Action, const FGameplayTag& InputTag, TFunction<void()> LegacyHandler)
{
	if(!EnhancedInputComponent || !Action)
	{
		return;
	}

	TWeakObjectPtr<AHeroCharacterBase> WeakThis(this);

	EnhancedInputComponent->BindActionValueLambda(Action, ETriggerEvent::Started, [WeakThis, InputTag, LegacyHandler](const FInputActionValue&)
	{
		AHeroCharacterBase* Self = WeakThis.Get();
		if(!Self)
		{
			return;
		}

		if(Self->TryAbilityInputPressed(InputTag))
		{
			return;
		}

		if(LegacyHandler)
		{
			LegacyHandler();
		}
	});

	EnhancedInputComponent->BindActionValueLambda(Action, ETriggerEvent::Completed, [WeakThis, InputTag](const FInputActionValue&)
	{
		if(AHeroCharacterBase* Self = WeakThis.Get())
		{
			Self->AbilityInputReleased(InputTag);
		}
	});
}

bool AHeroCharacterBase::TryAbilityInputPressed(const FGameplayTag& InputTag)
{
	UHeroAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	return ASC && ASC->AbilityInputTagPressed(InputTag);
}

void AHeroCharacterBase::AbilityInputReleased(const FGameplayTag& InputTag)
{
	if(UHeroAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->AbilityInputTagReleased(InputTag);
	}
}
