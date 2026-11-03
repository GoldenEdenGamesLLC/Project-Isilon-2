// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "EnemyCharacterBase.generated.h"

class UAbilitySystemComponent;
class UTagAttributeSet;
struct FOnAttributeChangeData;

/**
 * Shared GAS setup for every enemy. Unlike heroes, the ASC lives on the enemy itself
 * (no PlayerState) and only replicates tags/cues (Minimal mode).
 */
UCLASS(Abstract)
class PROJECT_ISILON_2_API AEnemyCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEnemyCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Abilities")
	UTagAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
	virtual void BeginPlay() override;

	// Server only. Sets MaxHealth and fills Health (used on spawn / pool reactivation).
	void InitHealthAttributes(float InMaxHealth);

	// Server only. Called whenever the GAS Health attribute changes.
	virtual void HandleHealthChanged(float OldHealth, float NewHealth) {}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UTagAttributeSet> AttributeSet;

private:
	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);
};
