// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyAIStats.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EGameDifficulty : uint8
{
	Easy		UMETA(DisplayName = "Easy"),
	Normal		UMETA(DisplayName = "Normal"),
	Hard		UMETA(DisplayName = "Hard")
};

UCLASS(BlueprintType)
class PROJECT_ISILON_2_API UEnemyAIStats : public UDataAsset
{
	GENERATED_BODY()
	
public:
	EGameDifficulty GetDifficulty() const
	{
		return Difficulty;
	}

	float GetDamageReceivedCoefficient() const
	{
		return DamageReceivedCoefficient;
	}

	float GetDamageDealtCoefficient() const
	{
		return DamageDealtCoefficient;
	}

	float GetEnemyHealthCoefficient() const
	{
		return EnemyHealthCoefficient;
	}

	float GetEnemyDamageCoefficient() const
	{
		return EnemyDamageCoefficient;
	}

	float GetEnemySpeedCoefficient() const
	{
		return EnemySpeedCoefficient;
	}
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty")
	EGameDifficulty Difficulty = EGameDifficulty::Normal; // Easy, Medium, Hard

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty|Player")
	float DamageReceivedCoefficient = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty|Player")
	float DamageDealtCoefficient = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty|Enemy")
	float EnemyHealthCoefficient = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty|Enemy")
	float EnemyDamageCoefficient = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty|Enemy")
	float EnemySpeedCoefficient = 1.0f;

private:
};
