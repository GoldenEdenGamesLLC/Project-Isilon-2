// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"
#include "EnemyCharacter.h"
#include "JumpNavLinkProxy.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"

#include "Engine/World.h"
#include "TimerManager.h"

void AEnemyAIController::OnPossess(APawn* InPawn){
    Super::OnPossess(InPawn);

    // for server only
    if(!HasAuthority()){
        return;
    }

    AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(InPawn);

    if(Enemy)
    {
        Enemy->LandedDelegate.AddUniqueDynamic(this, &AEnemyAIController::HandleEnemyLanded);
    }

    bPausedForPooling = false;
    bIsChasing = false;
    currTarget.Reset();

    StuckTimer = 0.0f;
    bHasLastPosition = false;
    LastPosition = FVector::ZeroVector;

    UpdateChase();

    GetWorldTimerManager().SetTimer(ChaseTimer, this, &AEnemyAIController::UpdateChase, ChaseCheckInterval, true);
}

void AEnemyAIController::OnUnPossess(){
    if(AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn()))
    {
        Enemy->LandedDelegate.RemoveDynamic(this, &AEnemyAIController::HandleEnemyLanded);
    }

    ActiveJumpLink.Reset();

    GetWorldTimerManager().ClearTimer(ChaseTimer);
    
    StopChasing();
    
    bPausedForPooling = false;
    
    Super::OnUnPossess();
}

void AEnemyAIController::UpdateChase()
{
    if(!HasAuthority() || bPausedForPooling)
    {
        return;
    }

    APawn* ControlledEnemy = GetPawn();
    if(!IsValid(ControlledEnemy)){
        return;
    }

    APawn* ClosestPlayer = FindClosestPlayer();
    if(!IsValid(ClosestPlayer)){
        StopChasing();
        return;
    }

    const float DistanceSquared = FVector::DistSquared(ControlledEnemy->GetActorLocation(), ClosestPlayer->GetActorLocation());
    const bool bAlreadyChasingThisPlayer = bIsChasing && currTarget.IsValid() && currTarget.Get() == ClosestPlayer;

    const float RequiredDistanceSquared = bAlreadyChasingThisPlayer ? LoseDistance : ChaseDistance;

    AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());
    const float AttackRange = Enemy->GetAttackRange();

    if(DistanceSquared > FMath::Square(RequiredDistanceSquared))
    {
        StopChasing();
        return;
    }

    if(DistanceSquared <= FMath::Square(AttackRange))
    {
        if(!bAlreadyChasingThisPlayer)
        {
            currTarget = ClosestPlayer;
            bIsChasing = true;
        }

        StopMovement();

        SetFocus(ClosestPlayer, EAIFocusPriority::Gameplay);
        Enemy->TryAttack(ClosestPlayer);

        return;
    }

    //check jump
    const FVector CurrentPosition = Enemy->GetActorLocation();

    if(!bHasLastPosition)
    {
        LastPosition = CurrentPosition;
        bHasLastPosition = true;
    }
    else
    {
        const float DistanceMoved = FVector::Dist2D(CurrentPosition, LastPosition);

        if(bAlreadyChasingThisPlayer /*&& DistanceMoved < MovementThreshold*/)
        {
            StuckTimer += ChaseCheckInterval;
        }
        else
        {
            StuckTimer = 0.0f;
        }

        LastPosition = CurrentPosition;
    }

    // if(StuckTimer >= StuckThresholdTime)
    // {
    //     if(TryJumpObstacle(Enemy, ClosestPlayer))
    //     {
    //         StuckTimer = 0.0f;
    //         return;
    //     }

    //     StuckTimer = 0.0f;
    //     return;
    // }

    if(!bAlreadyChasingThisPlayer)
    {
        currTarget = ClosestPlayer;
        bIsChasing = true;
    }

    ClearFocus(EAIFocusPriority::Gameplay);

    if(GetMoveStatus() != EPathFollowingStatus::Moving)
    {
        MoveToActor(ClosestPlayer, AcceptanceRadius, false, true, true, nullptr, true);
    }
}

APawn* AEnemyAIController::FindClosestPlayer() const
{
    // Find the closest player pawn
    const UWorld* world = GetWorld();
    if(!world)
    {
        return nullptr;
    }

    const APawn* ControlledEnemy = GetPawn();
    if(!ControlledEnemy)
    {
        return nullptr;
    }

    APawn* ClosestPlayer = nullptr;
    float ClosestDistanceSquared = TNumericLimits<float>::Max();
    for(FConstPlayerControllerIterator i = world->GetPlayerControllerIterator(); i; ++i)
    {
        APlayerController* PlayerController = i->Get();
        if(!IsValid(PlayerController))
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid player controller found"));
            continue;
        }

        APawn* PlayerPawn = PlayerController->GetPawn();
        if(!IsValid(PlayerPawn))
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid player pawn found"));
            continue;
        }
        
        const float DistanceSquared = FVector::DistSquared(ControlledEnemy->GetActorLocation(), PlayerPawn->GetActorLocation());

        //UE_LOG(LogTemp, Warning, TEXT("Player pawn %s found at distance %.2f"), *GetNameSafe(PlayerPawn), Distance);

        if(DistanceSquared < ClosestDistanceSquared)
        {
            ClosestDistanceSquared = DistanceSquared;
            ClosestPlayer = PlayerPawn;
        }
    }

    return ClosestPlayer;
}

void AEnemyAIController::StopChasing()
{
    StopMovement();
    ClearFocus(EAIFocusPriority::Gameplay);

    currTarget.Reset();
    bIsChasing = false;

    StuckTimer = 0.0f;
    bHasLastPosition = false;
}

void AEnemyAIController::PauseForPooling()
{
    if(!HasAuthority())
    {
		return;
	}

    bPausedForPooling = true;

    GetWorldTimerManager().ClearTimer(ChaseTimer);

    StopMovement();
    ClearFocus(EAIFocusPriority::Gameplay);

    currTarget.Reset();
    bIsChasing = false;
}

void AEnemyAIController::ResumeFromPooling()
{
    if(!HasAuthority())
    {
		return;
	}

    bPausedForPooling = false;

    currTarget.Reset();
    bIsChasing = false;

    StuckTimer = 0.0f;
    bHasLastPosition = false;
    LastPosition = FVector::ZeroVector;

    GetWorldTimerManager().ClearTimer(ChaseTimer);

    UpdateChase();

    GetWorldTimerManager().SetTimer(ChaseTimer, this, &AEnemyAIController::UpdateChase, ChaseCheckInterval, true);
}

bool AEnemyAIController::TryJumpObstacle(AEnemyCharacter* Enemy, APawn* Target)
{
    if(!IsValid(Enemy) || !IsValid(Target))
    {
        return false;
    }

    UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();

    if(!Movement || !Movement->IsMovingOnGround())
    {
        return false;
    }

    FVector JumpDirection = Target->GetActorLocation() - Enemy->GetActorLocation();
    JumpDirection.Z = 0.0f;

    if(!JumpDirection.Normalize())
    {
        return false;
    }

    UCapsuleComponent* Capsule = Enemy->GetCapsuleComponent();
    if(!Capsule)
    {
        return false;
    }

    const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();

    FVector LowStart = Enemy->GetActorLocation() - FVector(0.0f, 0.0f, CapsuleHalfHeight - 30.0f);
    LowStart += JumpDirection * (CapsuleRadius + 5.0f);

    const FVector LowEnd = LowStart + JumpDirection * ObstacleCheckDistance;

    FVector HighStart = Enemy->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
    HighStart += JumpDirection * (CapsuleRadius + 5.0f);

    const FVector HighEnd = HighStart + JumpDirection * ObstacleCheckDistance;
    
    FCollisionQueryParams QueryParams;
    
    QueryParams.AddIgnoredActor(Enemy);
    QueryParams.AddIgnoredActor(Target);

    FHitResult LowHit;
    FHitResult HighHit;

    const bool bLowBlocked = GetWorld()->LineTraceSingleByChannel(LowHit, LowStart, LowEnd, ECC_Visibility, QueryParams);
    const bool bHighBlocked = GetWorld()->LineTraceSingleByChannel(HighHit, HighStart, HighEnd, ECC_Visibility, QueryParams);

    if(bLowBlocked && !bHighBlocked)
    {
        StopMovement();
        const FVector LaunchVelocity = (JumpDirection * JumpForwardStrength) + FVector(0.0f, 0.0f, JumpZStrength);

        Enemy->LaunchCharacter(LaunchVelocity, true, true);
        return true;
    }

    return false;
}

//JUMP NAV LINK
void AEnemyAIController::SetActiveJumpLink(AJumpNavLinkProxy* JumpLink)
{
    ActiveJumpLink = JumpLink;
}

void AEnemyAIController::HandleEnemyLanded(const FHitResult& Hit)
{
    if(!HasAuthority())
    {
        return;
    }

    APawn* Enemy = GetPawn();

    if(ActiveJumpLink.IsValid() && IsValid(Enemy))
    {
        ActiveJumpLink->FinishJump(Enemy);
        ActiveJumpLink.Reset();
    }
}