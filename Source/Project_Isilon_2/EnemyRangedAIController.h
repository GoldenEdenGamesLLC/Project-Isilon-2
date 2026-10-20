// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyRangedAIController.generated.h"

class APawn;

/**
 * 
 */
UCLASS()
class PROJECT_ISILON_2_API AEnemyRangedAIController : public AAIController
{
	GENERATED_BODY()
public:
	void PauseForPooling();
	void ResumeFromPooling();

	APawn* GetCurrentTarget() const
	{
		return currTarget.Get();
	}

	float GetHoverHeight() const
	{
		return HoverHeight;
	}

	float GetAcceptanceRadius() const
	{
		return AcceptanceRadius;
	}

	float GetRingRadius() const
	{
		return RingRadius;
	}

	float GetRingAngleDegrees() const
	{
		return RingAngleDegrees;
	}

	bool IsInCastingRange() const
	{
		return bIsInCastingRange;
	}

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void StartChaseTimer();
	void UpdateMovement(APawn* Target);
	void UpdateChase();
	APawn* FindClosestPlayer() const;
	void StopChasing();

private:
	bool bIsChasing = false;
	bool bIsInCastingRange = false;

	bool bPausedForPooling = false;

	float NextTargetSearchTime = 0.0f;
	float NextPathRequestTime = 0.0f;

	float RingAngleDegrees = 0.0f;
	
	FTimerHandle ChaseTimer;
	TWeakObjectPtr<APawn> currTarget;
	TWeakObjectPtr<APawn> MovementTarget;

	UPROPERTY(EditDefaultsOnly, Category="AI|Pathing")
	float TargetSearchInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="AI|Pathing")
	float PathRetryInterval = 1.0f;

	//the distance at which an enemy will stop chasing a player
	UPROPERTY(EditDefaultsOnly, Category="AI|Flying", meta = (ClampMin = "0.0"))
	float LoseDistance = 12000.0f;

	UPROPERTY(EditDefaultsOnly, Category="AI|Flying", meta = (ClampMin = "0.0"))
	float ChaseDistance = 10000.0f;

	//how close the enemy will get to the player
	UPROPERTY(EditDefaultsOnly, Category="AI|Flying", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 100.0f;

	//how often the enemy checks if it should stop chasing
	//should not be every frame
	UPROPERTY(EditDefaultsOnly, Category="AI|Flying", meta = (ClampMin = "0.0"))
	float ChaseCheckInterval = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category="AI|Flying", meta = (ClampMin = "0.0"))
	float HoverHeight = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category="AI|Flying", meta = (ClampMin = "0.0"))
	float RingRadius = 200.0f;
};
