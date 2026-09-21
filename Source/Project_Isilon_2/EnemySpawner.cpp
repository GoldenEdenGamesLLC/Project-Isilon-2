// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemySpawner.h"
#include "EnemyCharacter.h"
#include "EnemyRangedCharacter.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"

// Sets default values
AEnemySpawner::AEnemySpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	
	SceneRoot->SetMobility(EComponentMobility::Movable);

	#if WITH_EDITORONLY_DATA
		EditorIcon = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorIcon"));
		
		EditorIcon->SetupAttachment(SceneRoot);
		EditorIcon->SetHiddenInGame(true);
	#endif
}

// Called when the game starts or when spawned
void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	
	// UE_LOG(LogTemp, Display, TEXT("EnemySpawner created: %s"), *GetPathName());

	//change to be 5-10 seconds after start of round or just slow spawning
	if(!HasAuthority()) return;

	InitializePools();

	if(bSpawnOnBeginPlay)
	{
		SpawnWave();
	}
}

void AEnemySpawner::InitializePools()
{
	if(!HasAuthority()) return;
	if(bPoolsInitialized) return;

	//safety check to ensure we don't run InitializePools multiple times
	bPoolsInitialized = true;

	if(!EnemyClass_1 || !EnemyClass_2)
	{
		UE_LOG(LogTemp, Error, TEXT("[POOL] Enemy classes are not configured."));
		return;
	}

	MeleeEnemyPool.Reset();
	RangedEnemyPool.Reset();

	//Melee
	for(int32 i = 0; i < InitialMeleePoolSize; i++)
	{
		APawn* spawnedMeleeEnemy = UAIBlueprintHelperLibrary::SpawnAIFromClass(
			this,
			EnemyClass_1,
			nullptr,
			GetActorLocation(),
			GetActorRotation(),
			true,					//handles collision
			this
		);

		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(spawnedMeleeEnemy);

		if(!IsValid(Enemy))
		{
			UE_LOG(LogTemp, Error, TEXT("[POOL] Failed to create melee enemy."));
			continue;
		}

		Enemy->SetOwningSpawner(this);
		Enemy->DeactivateForPool();

		MeleeEnemyPool.Add(Enemy);
	}

	//Ranged
	for(int32 i = 0; i < InitialRangedPoolSize; i++)
	{
		APawn* spawnedRangedEnemy = UAIBlueprintHelperLibrary::SpawnAIFromClass(
			this,
			EnemyClass_2,
			nullptr,
			GetActorLocation(),
			GetActorRotation(),
			true,					//handles collision
			this
		);

		AEnemyRangedCharacter* Enemy = Cast<AEnemyRangedCharacter>(spawnedRangedEnemy);

		if(!IsValid(Enemy))
		{
			UE_LOG(LogTemp, Error, TEXT("[POOL] Failed to create ranged enemy."));
			continue;
		}

		Enemy->SetOwningSpawner(this);
		Enemy->DeactivateForPool();

		RangedEnemyPool.Add(Enemy);
	}

	UE_LOG(LogTemp, Warning, TEXT("[POOL] created %d melee and %d ranged enemies."), MeleeEnemyPool.Num(), RangedEnemyPool.Num());
}

void AEnemySpawner::SpawnWave()
{
	if(!HasAuthority())
	{
		return;
	}

	if(!EnemyClass_1 || !EnemyClass_2)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyClass is not set in EnemySpawner %s"), *GetPathName());
		return;
	}
	
	//will need to change for sure for dynamic level spawning
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	
	if(!IsValid(NavigationSystem))
	{
		UE_LOG(LogTemp, Error, TEXT("[SPAWNER] NavigationSystem is Null."));
	}
	
	for(int32 i = 0; i < numEnemiesToSpawn; i++)
	{
		if(i % 2 == 0)
		{
			ActivatePooledMelee(NavigationSystem);
		}
		else
		{
			ActivatePooledRanged(NavigationSystem);
		}
	}
}

void AEnemySpawner::ActivatePooledMelee(UNavigationSystemV1* NavigationSystem)
{
	if(!IsValid(NavigationSystem))
	{
		return;
	}

	AEnemyCharacter* MeleeE = GetMeleeEnemyFromPool();

	if(!IsValid(MeleeE))
	{
		return;
	}

	FVector MeleeSpawn;

	bool bFoundMeleeSpawnLocation = FindValidSpawnLocation(NavigationSystem, MeleeSpawn);

	if(!bFoundMeleeSpawnLocation)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find spawn location for enemy in EnemySpawner %s"), *GetPathName());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SPAWN DEBUG] Spawner=%s, | NavPoint=%s | Offset=%.2f"), *GetActorLocation().ToString(), *MeleeSpawn.ToString(), SpawnHeightOffsetMelee);
	const FVector spawnLocation1 = MeleeSpawn + FVector::UpVector * SpawnHeightOffsetMelee;
	// Spawn the enemy at the calculated location

	UE_LOG(LogTemp, Warning, TEXT("[SPAWN DEBUG] CapsuleHalfHeight=%.2f | ActorScale=%s"), MeleeE->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), *MeleeE->GetActorScale3D().ToString());
	MeleeE->ActivateFromPool(spawnLocation1, GetActorRotation(), DifficultyStats, CalculateRuntimeDifficultyCoefficient());
}

AEnemyCharacter* AEnemySpawner::GetMeleeEnemyFromPool()
{
	for(TObjectPtr<AEnemyCharacter>& EMPtr : MeleeEnemyPool)
	{
		AEnemyCharacter* E = EMPtr.Get();
		if(IsValid(E) && !E->IsPoolActive())
		{
			return E;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[POOL] No available Melee Enemies"));

	return nullptr;
}

void AEnemySpawner::ReturnMeleeEnemyToPool(AEnemyCharacter* Enemy)
{
	if(!HasAuthority() || !IsValid(Enemy))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Returning MeleeEnemy:%s to pool."), *GetNameSafe(Enemy));
	Enemy->DeactivateForPool();

	FTimerHandle RespawnTimerHandle;
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AEnemySpawner::RespawnMeleeEnemy, RespawnDelay, false);
}

void AEnemySpawner::ActivatePooledRanged(UNavigationSystemV1* NavigationSystem)
{
	if(!IsValid(NavigationSystem))
	{
		return;
	}

	AEnemyRangedCharacter* RangedE = GetRangedEnemyFromPool();
	
	if(!IsValid(RangedE))
	{
		return;
	}

	FVector RangedSpawn;
	
	bool bFoundRangedSpawnLocation = FindValidSpawnLocation(NavigationSystem, RangedSpawn);

	if(!bFoundRangedSpawnLocation)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find spawn location for enemy in EnemySpawner %s"), *GetPathName());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SPAWN DEBUG] Spawner=%s, | NavPoint=%s | Offset=%.2f"), *GetActorLocation().ToString(), *RangedSpawn.ToString(), SpawnHeightOffsetRanged);

	const FVector spawnLocation2 = RangedSpawn + FVector::UpVector * SpawnHeightOffsetRanged;

	RangedE->ActivateFromPool(spawnLocation2, GetActorRotation(), DifficultyStats, CalculateRuntimeDifficultyCoefficient());
	// UE_LOG(LogTemp, Error, TEXT("[RANGED POOL] CURRENT SIZE = %d."), GetRangedPoolCount());
}

AEnemyRangedCharacter* AEnemySpawner::GetRangedEnemyFromPool()
{
	for(TObjectPtr<AEnemyRangedCharacter>& ERPtr : RangedEnemyPool)
	{
		AEnemyRangedCharacter* E = ERPtr.Get();
		if(IsValid(E) && !E->IsPoolActive())
		{
			return E;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[POOL] No available Ranged Enemies"));

	return nullptr;
}

void AEnemySpawner::ReturnRangedEnemyToPool(AEnemyRangedCharacter* Enemy)
{
	if(!HasAuthority() || !IsValid(Enemy))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Returning RangedEnemy:%s to pool."), *GetNameSafe(Enemy));
	Enemy->DeactivateForPool();

	FTimerHandle RespawnTimerHandle;
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AEnemySpawner::RespawnRangedEnemy, RespawnDelay, false);
}

bool AEnemySpawner::FindValidSpawnLocation(UNavigationSystemV1* NavigationSystem, FVector& OutSpawnLocation)
{
	if(!NavigationSystem)
	{
		return false;
	}

	FVector SpawnerLocation = GetActorLocation();
	constexpr int32 MaxAttempts = 10;
	constexpr float MaxVerticalDifference = 800.0f;
	
	for(int32 i = 0; i < MaxAttempts; ++i)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SPAWN DEBUG] Attempt=%d starting."), i)

		FNavLocation NavLocation;

		const bool bFoundPoint = NavigationSystem->GetRandomReachablePointInRadius(SpawnerLocation, SpawnRadius, NavLocation);

		if(!bFoundPoint)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SPAWN DEBUG] Attempt=%d failed, continuing."), i)
			continue;
		}

		const float VerticalDifference = FMath::Abs(NavLocation.Location.Z - SpawnerLocation.Z);
		UE_LOG(LogTemp, Warning, TEXT("[SPAWN DEBUG] Attempt=%d | Spawner=%s | NavPoint=%s | VerticalDifference=%.2f"), i, *SpawnerLocation.ToString(), *NavLocation.Location.ToString(), VerticalDifference);

		if(VerticalDifference > MaxVerticalDifference)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SPAWN DEBUG] Rejected nav point - vertical difference > 500."));
			continue;
		}
	
		OutSpawnLocation = NavLocation.Location;

		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SPAWN DEBUG] Failed to find spawn location."))

	return false;
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemySpawner::RespawnMeleeEnemy()
{
	if(!HasAuthority())
	{
		return;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if(!IsValid(NavigationSystem))
	{
		UE_LOG(LogTemp, Error, TEXT("[SPAWNER] NavigationSystem for Melee Enemies is Null."));
		return;
	}

	ActivatePooledMelee(NavigationSystem);
}

void AEnemySpawner::RespawnRangedEnemy()
{
	if(!HasAuthority())
	{
		return;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if(!IsValid(NavigationSystem))
	{
		UE_LOG(LogTemp, Error, TEXT("[SPAWNER] NavigationSystem for Ranged Enemies is NULL."));
	}

	ActivatePooledRanged(NavigationSystem);
}

// ===========================================
// TESTING
// ===========================================
int32 AEnemySpawner::GetActiveMeleeCount() const
{
	int32 count = 0;

	//count current melee enemies
	for(const AEnemyCharacter* Enemy : MeleeEnemyPool)
	{
		if(IsValid(Enemy) && Enemy->IsPoolActive())
		{
			count++;
		}
	}

	return count;
}

int32 AEnemySpawner::GetActiveRangedCount() const
{
	int32 count = 0;

	//count current ranged enemies
	for(const AEnemyRangedCharacter* Enemy : RangedEnemyPool)
	{
		if(IsValid(Enemy) && Enemy->IsPoolActive())
		{
			count++;
		}
	}

	return count;
}

float AEnemySpawner::CalculateRuntimeDifficultyCoefficient() const
{
	if(!GetWorld())
	{
		return 1.0f;
	}

	const float ElapsedSeconds = GetWorld()->GetTimeSeconds();
	const float ElapsedMinutes = ElapsedSeconds / 60.0f;

	const float RuntimeCoefficient = 1.0f + (ElapsedMinutes * DifficultyGrowthPerMinute);

	UE_LOG(LogTemp, Warning, TEXT("[DIFFICULTY] Seconds: %.2f | Minutes: %.2f | GrowthPerMinute: %.2f | RuntimeCoefficient: %.3f"),
									ElapsedSeconds, ElapsedMinutes, DifficultyGrowthPerMinute, RuntimeCoefficient);

	return FMath::Max(RuntimeCoefficient, 1.0f);
}