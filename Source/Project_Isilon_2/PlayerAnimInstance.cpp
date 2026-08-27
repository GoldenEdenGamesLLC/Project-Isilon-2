// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"

#include "CubeCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UPlayerAnimInstance::UPlayerAnimInstance()
{
    // Constructor implementation
}

void UPlayerAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    CubeCharacter = Cast<ACubeCharacter>(TryGetPawnOwner());

    if(IsValid(CubeCharacter)){
        CharacterMovement = CubeCharacter->GetCharacterMovement();
    }
}

void UPlayerAnimInstance::NativeUpdateAnimation(float deltaTime)
{
    Super::NativeUpdateAnimation(deltaTime);

    if (!IsValid(CubeCharacter))
	{
		CubeCharacter = Cast<ACubeCharacter>(TryGetPawnOwner());

		if (CubeCharacter)
		{
			CharacterMovement = CubeCharacter->GetCharacterMovement();
		}
	}

	if (!IsValid(CubeCharacter) || !IsValid(CharacterMovement))
	{
		Speed = 0.0f;
		bIsMoving = false;
		bIsAccelerating = false;
		bIsInAir = false;
		return;
	}

    FVector HorizontalVelocity = CubeCharacter->GetVelocity();
    HorizontalVelocity.Z = 0.0f;
    Speed = HorizontalVelocity.Size();
    bIsMoving = Speed > 3.0f;
    bIsAccelerating = !CharacterMovement->GetCurrentAcceleration().IsNearlyZero();
    bIsInAir = CharacterMovement->IsFalling();
}