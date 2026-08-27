// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"

#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

#include "Engine/World.h"
#include "TimerManager.h"

void AEnemyAIController::OnPossess(APawn* InPawn){
    Super::OnPossess(InPawn);

    // for server only
    if(!HasAuthority()){
        return;
    }

    bPausedForPooling = false;
    bIsChasing = false;
    currTarget.Reset();

    UpdateChase();

    GetWorldTimerManager().SetTimer(ChaseTimer, this, &AEnemyAIController::UpdateChase, ChaseCheckInterval, true);
}

void AEnemyAIController::OnUnPossess(){
    Super::OnUnPossess();

    GetWorldTimerManager().ClearTimer(ChaseTimer);

    StopChasing();

    bPausedForPooling = false;
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

    if(DistanceSquared > FMath::Square(RequiredDistanceSquared))
    {
        StopChasing();
        return;
    }

    if(!bAlreadyChasingThisPlayer)
    {
        currTarget = ClosestPlayer;
        bIsChasing = true;
    }

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

    GetWorldTimerManager().ClearTimer(ChaseTimer);

    UpdateChase();

    GetWorldTimerManager().SetTimer(ChaseTimer, this, &AEnemyAIController::UpdateChase, ChaseCheckInterval, true);
}