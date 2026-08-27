// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance.generated.h"

class ACubeCharacter;
class UCharacterMovementComponent;

/**
 * 
 */
UCLASS()
class PROJECT_ISILON_2_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float deltaTime) override;
public:
	UPlayerAnimInstance();

private:
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ACubeCharacter> CubeCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> CharacterMovement = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Animation", meta = (AllowPrivateAccess = "true"))
	bool bIsInAir = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Animation", meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Animation", meta = (AllowPrivateAccess = "true"))
	bool bIsMoving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Animation", meta = (AllowPrivateAccess = "true"))
	float Speed = 0.0f; //ACubeCharacter::GetSpeed();???
};
