// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DifficultyDataAsset.generated.h"

/**
 * Difficulty Coefficient for all enemy AI characters
 */
UCLASS()
class PROJECT_ISILON_2_API UDifficultyDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty")
	FText DifficultyName; // Easy, Medium, Hard

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty|Multipliers")
	float DamageReceivedCoefficient = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty|Multipliers")
	float DamageDealtCoefficient = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty|Multipliers")
	float EnemyHealthCoefficient = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty|Multipliers")
	float EnemySpeedCoefficient = 1.0f;

};
