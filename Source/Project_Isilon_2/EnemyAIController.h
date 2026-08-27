// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class APawn;

/**
 * Enemy AI Controller
 */
UCLASS()
class PROJECT_ISILON_2_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	void PauseForPooling();
	void ResumeFromPooling();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void UpdateChase();
	APawn* FindClosestPlayer() const;
	void StopChasing();

private:
	//the distance at which an enemy will stop chasing a player
	UPROPERTY(EditDefaultsOnly, Category="AI|Chase", meta = (ClampMin = "0.0"))
	float LoseDistance = 10000.0f;

	//distance that an enemy starts chasing ~ 12 meters
	UPROPERTY(EditDefaultsOnly, Category="AI|Chase", meta = (ClampMin = "0.0"))
	float ChaseDistance = 8500.0f;

	//how close the enemy will get to the player
	UPROPERTY(EditDefaultsOnly, Category="AI|Chase", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 25.0f;

	//how often the enemy checks if it should stop chasing
	//should not be every frame
	UPROPERTY(EditDefaultsOnly, Category="AI|Chase", meta = (ClampMin = "0.0"))
	float ChaseCheckInterval = 0.25f;

	FTimerHandle ChaseTimer;
	TWeakObjectPtr<APawn> currTarget;
	bool bIsChasing = false;

	bool bPausedForPooling = false;
};
