//Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyRangedAIController.h"

#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

void AEnemyRangedAIController::OnPossess(APawn* InPawn){
    Super::OnPossess(InPawn);

    if(!HasAuthority()){
        return;
    }

    RingAngleDegrees = FMath::FRandRange(0.0f, 360.0f);

    UpdateChase();

    GetWorldTimerManager().SetTimer(ChaseTimer, this, &AEnemyRangedAIController::UpdateChase, ChaseCheckInterval, true);
}

void AEnemyRangedAIController::OnUnPossess(){

    GetWorldTimerManager().ClearTimer(ChaseTimer);

    StopChasing();

    Super::OnUnPossess();
}

//who to chase
void AEnemyRangedAIController::UpdateChase()
{
    if(!HasAuthority()){
        return;
    }

    APawn* ControlledEnemy = GetPawn();
    if(!IsValid(ControlledEnemy)){
        return;
    }

    APawn* ClosestPlayer = FindClosestPlayer();
    if(!IsValid(ClosestPlayer)){
        currTarget.Reset();
        bIsChasing = false;
        
        return;
    }

    const float Distance = FVector::Distance(ControlledEnemy->GetActorLocation(), ClosestPlayer->GetActorLocation());
    //Acquiring Target
    const bool bSameTarget = currTarget.IsValid() && currTarget.Get() == ClosestPlayer;
    const float RequiredDistance = bSameTarget && bIsChasing ? LoseDistance : ChaseDistance;

    if(Distance > RequiredDistance)
    {
        currTarget.Reset();
        bIsChasing = false;
        bIsInAttackRange = false;

        return;
    }

    currTarget = ClosestPlayer;
    bIsChasing = true;

    //checks attack range to focus on player then check exit distance so attack channels until
    //player is out of range
    const bool bWasInAttackRange = bIsInAttackRange;
    if(bIsInAttackRange)
    {
        bIsInAttackRange = Distance <= AttackExitDistance;
    }
    else
    {
        bIsInAttackRange = Distance <= AttackEnterDistance;
    }

    //focuses on player when in attack range
    if(bIsInAttackRange)
    {
        SetFocus(ClosestPlayer, EAIFocusPriority::Gameplay);
    }
    else if(bWasInAttackRange)
    {
        ClearFocus(EAIFocusPriority::Gameplay);
    }
}

APawn* AEnemyRangedAIController::FindClosestPlayer() const
{
    // Find the closest player pawn
    const UWorld* world = GetWorld();
    if(!world)
    {
        UE_LOG(LogTemp, Error, TEXT("[RANGED FIND PLAYER] World is NULL"));
        return nullptr;
    }

    const APawn* ControlledEnemy = GetPawn();
    if(!ControlledEnemy)
    {
        UE_LOG(LogTemp, Error, TEXT("[RANGED FIND PLAYER] ControlledEnemy is Invalid."));
        return nullptr;
    }

    int32 ControllerCount = 0;
    int32 ValidPawnCount = 0;

    APawn* ClosestPlayer = nullptr;
    float ClosestDistanceSquared = TNumericLimits<float>::Max();
    for(FConstPlayerControllerIterator i = world->GetPlayerControllerIterator(); i; ++i)
    {
        ControllerCount++;
        APlayerController* PlayerController = i->Get();
        if(!IsValid(PlayerController))
        {
            UE_LOG(LogTemp, Warning, TEXT("[RANGED FIND PLAYER] PlayerController: %s"), *GetNameSafe(PlayerController));
            continue;
        }

        APawn* PlayerPawn = PlayerController->GetPawn();
        if(!IsValid(PlayerPawn))
        {
            UE_LOG(LogTemp, Warning, TEXT("[RANGED FIND PLAYER] PlayerPawn: %s"), *GetNameSafe(PlayerPawn));
            continue;
        }
        
        ValidPawnCount++;

        const float DistanceSquared = FVector::DistSquared(ControlledEnemy->GetActorLocation(), PlayerPawn->GetActorLocation());
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

void AEnemyRangedAIController::StopChasing()
{
    currTarget.Reset();
    bIsChasing = false;
}

void AEnemyRangedAIController::PauseForPooling()
{
    if(!HasAuthority())
    {
		return;
	}

    GetWorldTimerManager().ClearTimer(ChaseTimer);

    StopMovement();
    ClearFocus(EAIFocusPriority::Gameplay);

    currTarget.Reset();
    bIsChasing = false;
    bIsInAttackRange = false;
}

void AEnemyRangedAIController::ResumeFromPooling()
{
    if(!HasAuthority())
    {
		return;
	}

    currTarget.Reset();
    bIsChasing = false;

    GetWorldTimerManager().ClearTimer(ChaseTimer);

    UpdateChase();

    GetWorldTimerManager().SetTimer(ChaseTimer, this, &AEnemyRangedAIController::UpdateChase, ChaseCheckInterval, true);
}