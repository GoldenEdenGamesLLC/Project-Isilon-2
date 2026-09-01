// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class APawn;
class UEnemyAIStats;
class USceneComponent;
class UBillboardComponent;
class UNavigationSystemV1;

class AEnemyCharacter;
class AEnemyRangedCharacter;

UCLASS()
class PROJECT_ISILON_2_API AEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnemySpawner();

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawning")
	void SpawnWave();

	void ReturnMeleeEnemyToPool(AEnemyCharacter* Enemy);
	void ReturnRangedEnemyToPool(AEnemyRangedCharacter* Enemy);

	float CalculateRuntimeDifficultyCoefficient() const;

	virtual void Tick(float DeltaTime) override;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Difficulty")
	TObjectPtr<UEnemyAIStats> DifficultyStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Difficulty|Runtime")
	float DifficultyGrowthPerMinute = 0.10f;
private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	#if WITH_EDITORONLY_DATA
		UPROPERTY(VisibleAnywhere, Category = "Components")
		TObjectPtr<UBillboardComponent> EditorIcon;
	#endif

	UPROPERTY(EditAnywhere, Category = "Enemy Spawning")
	TSubclassOf<APawn> EnemyClass_1;

	UPROPERTY(EditAnywhere, Category = "Enemy Spawning")
	TSubclassOf<APawn> EnemyClass_2;

	UPROPERTY(EditAnywhere, Category = "Enemy Spawning", meta=(ClampMin=1))
	int32 numEnemiesToSpawn = 5;

	// ===========================================
	// SPAWN CIRCLE RADIUS
	// ===========================================

	UPROPERTY(EditAnywhere, Category = "Enemy Spawning", meta=(ClampMin=0.0f))
	float SpawnRadius = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy Spawning", meta=(ClampMin=0.0f))
	float SpawnHeightOffsetMelee = 100.0f;

	//problem if using MoveToActor - thats only meant for being on the ground
	UPROPERTY(EditAnywhere, Category = "Enemy Spawning", meta=(ClampMin=0.0f))
	float SpawnHeightOffsetRanged = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy Spawning")
	bool bSpawnOnBeginPlay = true;

	// ===========================================
	// OBJECT POOLING
	// ===========================================

	UPROPERTY(EditDefaultsOnly, Category="Object Pooling")
	int32 InitialMeleePoolSize = 25;

	UPROPERTY(EditDefaultsOnly, Category="Object Pooling")
	int32 InitialRangedPoolSize = 25;

	UPROPERTY(EditDefaultsOnly, Category="Object Pooling")
	float RespawnDelay = 3.0f;

	UPROPERTY()
	TArray<TObjectPtr<AEnemyCharacter>> MeleeEnemyPool;

	UPROPERTY()
	TArray<TObjectPtr<AEnemyRangedCharacter>> RangedEnemyPool;

	void InitializePools();
	bool bPoolsInitialized = false;

	void ActivatePooledMelee(UNavigationSystemV1* NavigationSystem);
	void RespawnMeleeEnemy();
	AEnemyCharacter* GetMeleeEnemyFromPool();

	void ActivatePooledRanged(UNavigationSystemV1* NavigationSystem);
	void RespawnRangedEnemy();
	AEnemyRangedCharacter* GetRangedEnemyFromPool();
	
	public:
	// ===========================================
	// TESTING
	// ===========================================
	int32 GetMeleePoolCount() const 
	{
		return MeleeEnemyPool.Num();
	}

	int32 GetRangedPoolCount() const 
	{
		return RangedEnemyPool.Num();
	}

	int32 GetActiveMeleeCount() const;
	int32 GetActiveRangedCount() const;
};
