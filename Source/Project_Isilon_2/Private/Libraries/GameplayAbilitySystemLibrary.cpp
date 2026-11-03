// Fill out your copyright notice in the Description page of Project Settings.


#include "Libraries/GameplayAbilitySystemLibrary.h"

#include "Data/GameplayTags.h"
#include "AbilitySystem/GE_Damage.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Controller.h"
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

bool UGameplayAbilitySystemLibrary::ApplyGASDamage(AActor* Target, AActor* DamageCauser, AController* Instigator, const TMap<FGameplayTag, float>& DamageByType, TSubclassOf<UGameplayEffect> DamageEffectClass)
{
    if(!IsValid(Target) || !Target->HasAuthority() || DamageByType.IsEmpty())
    {
        return false;
    }

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
    if(!TargetASC)
    {
        return false;
    }

    UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(DamageCauser);
    if(!SourceASC && Instigator)
    {
        SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator->GetPawn());
    }

    // No attacker ASC (e.g. environmental damage): apply from the target itself.
    // The source stats used by the calculation will then be the target's own.
    UAbilitySystemComponent* ApplyingASC = SourceASC ? SourceASC : TargetASC;

    if(!DamageEffectClass)
    {
        DamageEffectClass = UGE_Damage::StaticClass();
    }

    FGameplayEffectContextHandle Context = ApplyingASC->MakeEffectContext();
    Context.AddInstigator(Instigator ? Instigator->GetPawn() : DamageCauser, DamageCauser);

    const FGameplayEffectSpecHandle SpecHandle = ApplyingASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
    if(!SpecHandle.IsValid())
    {
        return false;
    }

    for(const TPair<FGameplayTag, float>& Pair : DamageByType)
    {
        SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
    }

    ApplyingASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
    return true;
}
