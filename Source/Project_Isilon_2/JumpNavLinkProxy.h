// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/NavLinkProxy.h"
#include "JumpNavLinkProxy.generated.h"

/**
 * 
 */

class UNavLinkCustomComponent;
class AEnemyCharacter;
class AEnemyAIController;

UCLASS()
class PROJECT_ISILON_2_API AJumpNavLinkProxy : public ANavLinkProxy
{
	GENERATED_BODY()
	
public:
	AJumpNavLinkProxy();
	void FinishJump(AActor* Agent);

protected:
	void HandleSmartLinkReached(UNavLinkCustomComponent* LinkComp, UObject* PathingAgent, const FVector& DestPoint);

	bool CalculateJumpVelocity(AEnemyCharacter* Enemy, const FVector& Destination, FVector& OutVelocity) const;
	
protected:
	UPROPERTY(EditAnywhere, Category="Jump")
	float JumpApexHeight = 150.0f;

	UPROPERTY(EditAnywhere, Category="Jump")
	float MaxHorizontalJumpSpeed = 1000.0f;
};
