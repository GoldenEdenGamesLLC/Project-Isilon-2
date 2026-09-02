// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class AEnemySpawner;
class UEnemyAIStats;

UCLASS()
class PROJECT_ISILON_2_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="Stats")
	float MeleeEnemyBaseHealth = 150.0f;

	UPROPERTY(ReplicatedUsing = OnRep_MeleeEnemyCurrentHealth, VisibleAnywhere, BlueprintReadOnly, Category="Stats")
	float MeleeEnemyCurrentHealth = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category="Stats")
	float BaseDamage = 28.0f;

	UPROPERTY(ReplicatedUsing = OnRep_MaxHealth, VisibleAnywhere, BlueprintReadOnly, Category="Stats")
	float MaxHealth;
	float Damage;

	void ApplyDifficulty(const UEnemyAIStats* DifficultyStats, float RuntimeCoefficient);

	UFUNCTION()
	void OnRep_MeleeEnemyCurrentHealth();

	UFUNCTION()
	void OnRep_MaxHealth();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_PoolActive)
	bool bPoolActive = false;

	UFUNCTION()
	void OnRep_PoolActive();

	void ApplyPoolState();

	TArray<uint16> HealthVariations = { 300, 275, 250, 200, 150 };
	TArray<uint16> SpeedsVariations = { 100, 200, 300, 400, 500 };
	int16 RandomIndex = 0;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// ===========================================
	// OBJECT POOLING
	// ===========================================
	void ActivateFromPool(const FVector& SpawnLocation, const FRotator& SpawnRotation, const UEnemyAIStats* DifficultyStats, float RuntimeCoefficient);

	void DeactivateForPool();

	bool IsPoolActive() const
	{
		return bPoolActive;
	}

	void SetOwningSpawner(AEnemySpawner* Spawner)
	{
		OwningSpawner = Spawner;
	}

private:
	UPROPERTY()
	TObjectPtr<AEnemySpawner> OwningSpawner;
};
