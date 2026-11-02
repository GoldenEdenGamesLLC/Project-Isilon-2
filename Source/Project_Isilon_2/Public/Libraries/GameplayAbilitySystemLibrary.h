// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NativeGameplayTags.h"
#include "GameplayAbilitySystemLibrary.generated.h"


/**
 * 
 */
UCLASS()
class PROJECT_ISILON_2_API UGameplayAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure)
	static FGameplayTag GetRandomTagFromContainer(const FGameplayTagContainer& Container);

	UFUNCTION(BlueprintCallable)
	static FGameplayTagContainer GetAllChildrenTagsOfCategories(const FGameplayTagContainer& CategoryTags);

	UFUNCTION(BlueprintPure)
	static FGameplayTagContainer GetDamageTypeTags();
};
