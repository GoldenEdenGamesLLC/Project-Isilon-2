// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"

#include "BarbarianCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UPlayerAnimInstance::UPlayerAnimInstance()
{
    // Constructor implementation
}

void UPlayerAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    BarbarianCharacter = Cast<ABarbarianCharacter>(TryGetPawnOwner());

    if(IsValid(BarbarianCharacter)){
        CharacterMovement = BarbarianCharacter->GetCharacterMovement();
    }
}

void UPlayerAnimInstance::NativeUpdateAnimation(float deltaTime)
{
    Super::NativeUpdateAnimation(deltaTime);

    if (!IsValid(BarbarianCharacter))
	{
		BarbarianCharacter = Cast<ABarbarianCharacter>(TryGetPawnOwner());

		if (BarbarianCharacter)
		{
			CharacterMovement = BarbarianCharacter->GetCharacterMovement();
		}
	}

	if (!IsValid(BarbarianCharacter) || !IsValid(CharacterMovement))
	{
		Speed = 0.0f;
		bIsMoving = false;
		bIsAccelerating = false;
		bIsInAir = false;
		return;
	}

    FVector HorizontalVelocity = BarbarianCharacter->GetVelocity();
    HorizontalVelocity.Z = 0.0f;
    Speed = HorizontalVelocity.Size();
    bIsMoving = Speed > 3.0f;
    bIsAccelerating = !CharacterMovement->GetCurrentAcceleration().IsNearlyZero();
    bIsInAir = CharacterMovement->IsFalling();
}