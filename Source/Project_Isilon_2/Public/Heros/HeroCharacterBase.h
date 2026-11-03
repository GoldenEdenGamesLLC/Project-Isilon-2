// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "HeroCharacterBase.generated.h"

class UHeroAbilitySystemComponent;
class UTagAttributeSet;
class UHeroData;
class UEnhancedInputComponent;
class UInputAction;
struct FOnAttributeChangeData;

/**
 * Shared base for every playable hero (Barbarian, Ranger, Wizard, ...).
 * The ASC lives on AHeroPlayerState; this class wires it up when possessed
 * and applies the hero's UHeroData on the server.
 */
UCLASS(Abstract)
class PROJECT_ISILON_2_API AHeroCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AHeroCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Abilities")
	UHeroAbilitySystemComponent* GetHeroAbilitySystemComponent() const { return AbilitySystemComponent.Get(); }

	UFUNCTION(BlueprintPure, Category = "Abilities")
	UTagAttributeSet* GetAttributeSet() const { return AttributeSet.Get(); }

	const UHeroData* GetHeroData() const { return HeroData; }

	// Routes all UGameplayStatics::ApplyDamage hits into GAS as Physical damage
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	// Server
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	// Client
	virtual void OnRep_PlayerState() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Binds an input action to an ability input tag.
	 * Pressed: activates the ability granted with InputTag; if the hero has none yet, calls LegacyHandler instead.
	 * Released: forwards the release to the ability.
	 * Lets each input move to GAS on its own, just by adding the ability to the hero's UHeroData.
	 */
	void BindAbilityInput(UEnhancedInputComponent* EnhancedInputComponent, const UInputAction* Action, const FGameplayTag& InputTag, TFunction<void()> LegacyHandler = nullptr);

	// Returns true if an ability handled this input
	bool TryAbilityInputPressed(const FGameplayTag& InputTag);
	void AbilityInputReleased(const FGameplayTag& InputTag);

	// Starting Health / MaxHealth used when HeroData has no InitAttributesEffect
	virtual void GetDefaultHealth(float& OutHealth, float& OutMaxHealth) const;

	// Server only. Called whenever the GAS Health attribute changes.
	virtual void HandleHealthChanged(float OldHealth, float NewHealth) {}

	// Abilities, attributes and class tag for this hero. Set on each hero Blueprint.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UHeroData> HeroData;

private:
	void InitAbilityActorInfo();
	void UnbindHealthChanged();
	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);

	TWeakObjectPtr<UHeroAbilitySystemComponent> AbilitySystemComponent;
	TWeakObjectPtr<UTagAttributeSet> AttributeSet;

	FDelegateHandle HealthChangedHandle;
};
