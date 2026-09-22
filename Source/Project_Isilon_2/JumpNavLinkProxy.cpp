// Fill out your copyright notice in the Description page of Project Settings.


#include "JumpNavLinkProxy.h"

#include "EnemyCharacter.h"
#include "EnemyAIController.h"

#include "NavLinkCustomComponent.h"
#include "Navigation/PathFollowingComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

AJumpNavLinkProxy::AJumpNavLinkProxy()
{
    bSmartLinkIsRelevant = true;

    if(GetSmartLinkComp())
    {
        GetSmartLinkComp()->SetMoveReachedLink(this, &AJumpNavLinkProxy::HandleSmartLinkReached);
    }
}

void AJumpNavLinkProxy::HandleSmartLinkReached(UNavLinkCustomComponent* LinkComp, UObject* PathingAgent, const FVector& DestPoint)
{
    UPathFollowingComponent* PathComp = Cast<UPathFollowingComponent>(PathingAgent);

    if(!PathComp)
    {
        return;
    }

    AEnemyAIController* EnemyController = Cast<AEnemyAIController>(PathComp->GetOwner());

    if(!EnemyController)
    {
        return;
    }

    AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(EnemyController->GetPawn());

    if(!Enemy)
    {
        return;
    }

    FVector LaunchVelocity;

    if(!CalculateJumpVelocity(Enemy, DestPoint, LaunchVelocity))
    {
        UE_LOG(LogTemp, Warning, TEXT("[JUMP LINK] Could not calculate jump for %s"), *GetNameSafe(Enemy));

        ResumePathFollowing(Enemy);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[JUMP LINK] %s jumping\n Start: %s\n Destination: %s\n Velocity: %s"), *GetNameSafe(Enemy), *Enemy->GetActorLocation().ToString(), *DestPoint.ToString(), *LaunchVelocity.ToString());

    Enemy->GetCharacterMovement()->StopMovementImmediately();
    EnemyController->SetActiveJumpLink(this);
    Enemy->LaunchCharacter(LaunchVelocity, true, true);
}

bool AJumpNavLinkProxy::CalculateJumpVelocity(AEnemyCharacter* Enemy, const FVector& Destination, FVector& OutVelocity) const
{
    if(!Enemy)
    {
        return false;
    }

    UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
    UCapsuleComponent* Capsule = Enemy->GetCapsuleComponent();

    if(!Movement || !Capsule)
    {
        return false;
    }

    const FVector Start = Enemy->GetActorLocation();

    FVector End = Destination;
    End.Z += Capsule->GetScaledCapsuleHalfHeight();

    // UE Gravity set = -980 cm/s^2
    const float GravityZ = GetWorld()->GetGravityZ() * Movement->GravityScale;
    const float Gravity = FMath::Abs(GravityZ);

    if(Gravity <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    //  Apex above both platforms
    const float ApexZ = FMath::Max(Start.Z, End.Z) + JumpApexHeight;
    const float HeightToApex = ApexZ - Start.Z;
    const float HeightFromApex = ApexZ - End.Z;

    //  Physics equation:
    //  -- distance = 1/2 * gravity * time^2
    //  Solving for time:
    //  -- time = sqrt(2 * distance / gravity)

    const float TimeUp = FMath::Sqrt((2.0f * HeightToApex) / Gravity);
    const float TimeDown = FMath::Sqrt((2 * HeightFromApex) / Gravity);
    const float TotalTime = TimeUp + TimeDown;

    if(TotalTime <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    //  Horizontal velocity calculation to reach destination during TotalTime
    FVector HorizontalDelta = End - Start;
    HorizontalDelta.Z = 0.0f;

    FVector HorizontalVelocity = HorizontalDelta / TotalTime;

    //  Vertical velocity required to reach our apex
    //  Velocity.Z = 0
    //  therefore
    //  InitialiVelocityZ = Gravity * TimeUp
    const float VerticalVelocity = Gravity * TimeUp;

    if(HorizontalVelocity.Size() > MaxHorizontalJumpSpeed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[JUMP LINK] Required horizontal speed = %.2f\n > max = %.2f"), HorizontalVelocity.Size(), MaxHorizontalJumpSpeed);
        return false;
    }

    OutVelocity = HorizontalVelocity + FVector(0.0f, 0.0f, VerticalVelocity);

    return true;
}

void AJumpNavLinkProxy::FinishJump(AActor* Agent)
{
    if(!IsValid(Agent))
    {
        return;
    }

    ResumePathFollowing(Agent);
}