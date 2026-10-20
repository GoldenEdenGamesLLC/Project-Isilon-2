//Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyRangedAIController.h"
#include "EnemyRangedCharacter.h"

#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

void AEnemyRangedAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if(!HasAuthority()){
        return;
    }

    bPausedForPooling = false;

    StopChasing();

    NextTargetSearchTime = 0.0f;
    NextPathRequestTime = 0.0f;
    
    RingAngleDegrees = FMath::FRandRange(0.0f, 360.0f);

    StartChaseTimer();
}

void AEnemyRangedAIController::OnUnPossess()
{
    GetWorldTimerManager().ClearTimer(ChaseTimer);
    
    if(HasAuthority())
    {
        StopChasing();
    }
    
    bPausedForPooling = false;

    Super::OnUnPossess();
}

//who to chase
void AEnemyRangedAIController::UpdateChase()
{
    if(!HasAuthority() || bPausedForPooling)
    {
        return;
    }

    AEnemyRangedCharacter* RangedEnemy = Cast<AEnemyRangedCharacter>(GetPawn());
    if(!IsValid(RangedEnemy))
    {
        return;
    }

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    const FVector EnemyLocation = RangedEnemy->GetActorLocation();

    // APawn* ClosestPlayer = FindClosestPlayer();
    APawn* ClosestPlayer = currTarget.Get();
    if(!IsValid(ClosestPlayer))
    {
        if(CurrentTime < NextTargetSearchTime)
        {
            return;
        }

        NextTargetSearchTime = CurrentTime + TargetSearchInterval;

        ClosestPlayer = FindClosestPlayer();

        if(!IsValid(ClosestPlayer))
        {
            StopChasing();            
            return;
        }
    }

    const float DistanceSquared = FVector::DistSquared(RangedEnemy->GetActorLocation(), ClosestPlayer->GetActorLocation());
    const bool bSameTarget = currTarget.IsValid() && currTarget.Get() == ClosestPlayer;
    const float RequiredDistance = bSameTarget && bIsChasing ? LoseDistance : ChaseDistance;

    const float CastRange = RangedEnemy->GetCastEnterDistance();

    if(DistanceSquared > FMath::Square(RequiredDistance))
    {
        StopChasing();
        
        NextTargetSearchTime = CurrentTime + TargetSearchInterval;

        return;
    }

    if(!bSameTarget)
    {
        StopChasing();

        currTarget = ClosestPlayer;
        NextPathRequestTime = 0.0f;
    }

    bIsChasing = true;

    //checks attack range to focus on player then check exit distance so attack channels until
    //player is out of range
    const bool bWasInCastingRange = bIsInCastingRange;
    const float CastEnterDistance = RangedEnemy->GetCastEnterDistance();
    const float CastExitDistance = RangedEnemy->GetCastExitDistance();

    if(bIsInCastingRange)
    {
        bIsInCastingRange = DistanceSquared <= FMath::Square(CastExitDistance);
    }
    else
    {
        bIsInCastingRange = DistanceSquared <= FMath::Square(CastEnterDistance);
    }

    //focuses on player when in attack range
    if(bIsInCastingRange)
    {
        if(GetMoveStatus() == EPathFollowingStatus::Moving)
        {
            StopMovement();
        }

        MovementTarget.Reset();

        if(!bWasInCastingRange || !bSameTarget)
        {
            SetFocus(ClosestPlayer, EAIFocusPriority::Gameplay);
        }

        RangedEnemy->TryCast(ClosestPlayer);
    }
    else
    {
        if(bWasInCastingRange)
        {
            ClearFocus(EAIFocusPriority::Gameplay);

        }

        UpdateMovement(ClosestPlayer);
    }
}

void AEnemyRangedAIController::UpdateMovement(APawn* Target)
{
    if(!IsValid(Target))
    {
        return;
    }
    
    const bool bSameMovementTarget = MovementTarget.Get() == Target;
    const bool bMoving = GetMoveStatus() == EPathFollowingStatus::Moving;

    if(bSameMovementTarget && bMoving)
    {
        return;
    }

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if(CurrentTime < NextPathRequestTime)
    {
        return;
    }

    AEnemyRangedCharacter* RangedEnemy = Cast<AEnemyRangedCharacter>(GetPawn());
    if(!IsValid(RangedEnemy))
    {
        return;
    }

    const float DesiredAcceptanceRadius = FMath::Max(AcceptanceRadius, RangedEnemy->GetCastEnterDistance() * 0.85f);
    const EPathFollowingRequestResult::Type Result = MoveToActor(Target, DesiredAcceptanceRadius, false, true, true, nullptr, false);

    NextPathRequestTime = CurrentTime + PathRetryInterval;

    if(Result != EPathFollowingRequestResult::Failed)
    {
        MovementTarget = Target;
    }
    else
    {
        MovementTarget.Reset();
    }
}

APawn* AEnemyRangedAIController::FindClosestPlayer() const
{
    // Find the closest player pawn
    const UWorld* world = GetWorld();
    if(!world)
    {
        //UE_LOG(LogTemp, Error, TEXT("[RANGED FIND PLAYER] World is NULL"));
        return nullptr;
    }

    const APawn* ControlledEnemy = GetPawn();
    if(!ControlledEnemy)
    {
        //UE_LOG(LogTemp, Error, TEXT("[RANGED FIND PLAYER] ControlledEnemy is Invalid."));
        return nullptr;
    }

    const FVector EnemyLocation = ControlledEnemy->GetActorLocation();

    APawn* ClosestPlayer = nullptr;
    float ClosestDistanceSquared = TNumericLimits<float>::Max();
    for(FConstPlayerControllerIterator i = world->GetPlayerControllerIterator(); i; ++i)
    {
        const APlayerController* PlayerController = i->Get();
        if(!IsValid(PlayerController))
        {
            //UE_LOG(LogTemp, Warning, TEXT("[RANGED FIND PLAYER] PlayerController: %s"), *GetNameSafe(PlayerController));
            continue;
        }

        APawn* PlayerPawn = PlayerController->GetPawn();
        if(!IsValid(PlayerPawn))
        {
            //UE_LOG(LogTemp, Warning, TEXT("[RANGED FIND PLAYER] PlayerPawn: %s"), *GetNameSafe(PlayerPawn));
            continue;
        }
        
        const float DistanceSquared = FVector::DistSquared(EnemyLocation, PlayerPawn->GetActorLocation());
        const float Distance = FMath::Sqrt(DistanceSquared);

        //UE_LOG(LogTemp, Warning, TEXT("Player pawn %s found at distance %.2f"), *GetNameSafe(PlayerPawn), Distance);

        if(DistanceSquared < ClosestDistanceSquared)
        {
            ClosestDistanceSquared = DistanceSquared;
            ClosestPlayer = PlayerPawn;
        }
    }

    return ClosestPlayer;
}

void AEnemyRangedAIController::StartChaseTimer()
{
    GetWorldTimerManager().ClearTimer(ChaseTimer);

    if(!HasAuthority() || !GetPawn() || bPausedForPooling)
    {
        return;
    }

    const float Interval = FMath::Max(0.05f, ChaseCheckInterval);

    const float InitialDelay = FMath::FRandRange(0.01f, ChaseCheckInterval);

    GetWorldTimerManager().SetTimer(ChaseTimer, this, &AEnemyRangedAIController::UpdateChase, Interval, true, InitialDelay);
}

void AEnemyRangedAIController::StopChasing()
{
    if(GetMoveStatus() == EPathFollowingStatus::Moving)
    {
        StopMovement();
    }

    if(bIsInCastingRange)
    {
        ClearFocus(EAIFocusPriority::Gameplay);
    }

    currTarget.Reset();
    MovementTarget.Reset();

    bIsChasing = false;
    bIsInCastingRange = false;
}

//Begin Pooling
void AEnemyRangedAIController::PauseForPooling()
{
    if(!HasAuthority())
    {
		return;
	}

    bPausedForPooling = true;

    GetWorldTimerManager().ClearTimer(ChaseTimer);

    StopChasing();

    NextTargetSearchTime = 0.0f;
    NextPathRequestTime = 0.0f;
}

void AEnemyRangedAIController::ResumeFromPooling()
{
    if(!HasAuthority())
    {
		return;
	}

    bPausedForPooling = false;

    StopChasing();

    NextTargetSearchTime = 0.0f;
    NextPathRequestTime = 0.0f;

    StartChaseTimer();
}
//END POOLING