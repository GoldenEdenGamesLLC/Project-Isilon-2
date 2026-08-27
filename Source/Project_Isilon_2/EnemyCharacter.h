// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class AEnemySpawner;

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

	UPROPERTY(EditDefaultsOnly, Category="Health")
	float MeleeEnemyBaseHealth = 150.0f;

	UPROPERTY(ReplicatedUsing = OnRep_MeleeEnemyCurrentHealth, VisibleAnywhere, BlueprintReadOnly, Category="Health")
	float MeleeEnemyCurrentHealth = 150.0f;

	UFUNCTION()
	void OnRep_MeleeEnemyCurrentHealth();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_PoolActive)
	bool bPoolActive = false;

	UFUNCTION()
	void OnRep_PoolActive();

	void ApplyPoolState();
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// ===========================================
	// OBJECT POOLING
	// ===========================================
	void ActivateFromPool(const FVector& SpawnLocation, const FRotator& SpawnRotation);

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
