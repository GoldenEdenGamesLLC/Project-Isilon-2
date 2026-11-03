// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Damage.generated.h"

/**
 * Default instant damage effect: runs UDamageExecCalc.
 * Set the damage with SetByCaller magnitudes keyed by damage type tag.
 * Make a Blueprint child if you want to add cues or tags to every hit.
 */
UCLASS()
class PROJECT_ISILON_2_API UGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Damage();
};
