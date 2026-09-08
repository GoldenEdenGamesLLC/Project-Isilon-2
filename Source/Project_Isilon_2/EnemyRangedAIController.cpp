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

    RingAngleDegrees = FMath::FRandRange(0.0f, 360.0f);

    UpdateChase();

    GetWorldTimerManager().SetTimer(ChaseTimer, this, &AEnemyRangedAIController::UpdateChase, ChaseCheckInterval, true);
}

void AEnemyRangedAIController::OnUnPossess()
{
    Super::OnUnPossess();

    GetWorldTimerManager().ClearTimer(ChaseTimer);

    StopChasing();

    bPausedForPooling = false;
}

//who to chase
void AEnemyRangedAIController::UpdateChase()
{
    if(!HasAuthority() || bPausedForPooling){
        return;
    }

    AEnemyRangedCharacter* RangedEnemy = Cast<AEnemyRangedCharacter>(GetPawn());
    if(!IsValid(RangedEnemy)){
        return;
    }

    APawn* ClosestPlayer = FindClosestPlayer();
    if(!IsValid(ClosestPlayer)){
        currTarget.Reset();
        bIsChasing = false;
        
        return;
    }

    const float DistanceSquared = FVector::DistSquared(RangedEnemy->GetActorLocation(), ClosestPlayer->GetActorLocation());
    //Acquiring Target
    const bool bSameTarget = currTarget.IsValid() && currTarget.Get() == ClosestPlayer;
    const float RequiredDistance = bSameTarget && bIsChasing ? LoseDistance : ChaseDistance;

    const float CastRange = RangedEnemy->GetCastEnterDistance();

    if(DistanceSquared > FMath::Square(RequiredDistance))
    {
        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
        
        currTarget.Reset();
        
        bIsChasing = false;
        bIsInCastingRange = false;

        return;
    }

    currTarget = ClosestPlayer;
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
        SetFocus(ClosestPlayer, EAIFocusPriority::Gameplay);
        RangedEnemy->TryCast(ClosestPlayer);
    }
    
    if(bWasInCastingRange)
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

//Begin Pooling
void AEnemyRangedAIController::PauseForPooling()
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
    bIsInCastingRange = false;
}

void AEnemyRangedAIController::ResumeFromPooling()
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

    GetWorldTimerManager().SetTimer(ChaseTimer, this, &AEnemyRangedAIController::UpdateChase, ChaseCheckInterval, true);
}
//END POOLING

//Start 