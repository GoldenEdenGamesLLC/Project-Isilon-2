// Fill out your copyright notice in the Description page of Project Settings.


#include "Libraries/GameplayAbilitySystemLibrary.h"

#include "Data/GameplayTags.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

FGameplayTag UGameplayAbilitySystemLibrary::GetRandomTagFromContainer(const FGameplayTagContainer& Container)
{
    if(Container.IsEmpty())
    {
        return FGameplayTag::EmptyTag;
    }

    const int32 RandomIndex = FMath::RandRange(0, Container.Num() - 1);
    return Container.GetByIndex(RandomIndex);
}

FGameplayTagContainer UGameplayAbilitySystemLibrary::GetAllChildrenTagsOfCategories(const FGameplayTagContainer &CategoryTags)
{
    FGameplayTagContainer AllChildrenTags;
    const UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

    for (const FGameplayTag& CategoryTag : CategoryTags)
    {
        AllChildrenTags.AppendTags(TagsManager.RequestGameplayTagChildren(CategoryTag));
    }

    return AllChildrenTags;
}

FGameplayTagContainer UGameplayAbilitySystemLibrary::GetDamageTypeTags()
{
    const UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();
    return TagsManager.RequestGameplayTagChildren(GameplayTags::Combat::DamageTypes::DamageTypeParent);
}