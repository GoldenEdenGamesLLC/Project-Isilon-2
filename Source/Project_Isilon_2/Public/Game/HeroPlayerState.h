// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "HeroPlayerState.generated.h"

class UHeroAbilitySystemComponent;
class UTagAttributeSet;

/**
 * Owns the player's ASC and attributes so they persist across hero death/respawn.
 */
UCLASS()
class PROJECT_ISILON_2_API AHeroPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AHeroPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UHeroAbilitySystemComponent* GetHeroAbilitySystemComponent() const { return AbilitySystemComponent; }
	UTagAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UHeroAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UTagAttributeSet> AttributeSet;
};
