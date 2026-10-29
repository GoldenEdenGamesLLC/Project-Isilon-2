//Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyRangedAIController.h"
#include "EnemyRangedCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

AEnemyRangedAIController::AEnemyRangedAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

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
    GetWorldTimerManager().ClearTimer(RangedMovementTimerHandle);
    
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

        UpdateMovement();
    }
}

void AEnemyRangedAIController::UpdateMovement()
{
    APawn* Target = currTarget.Get();
    if(!IsValid(Target))
    {
        return;
    }

    AEnemyRangedCharacter* RangedEnemy = Cast<AEnemyRangedCharacter>(GetPawn());
    if(!IsValid(RangedEnemy))
    {
        return;
    }

	UCharacterMovementComponent* Movement = RangedEnemy->GetCharacterMovement();
	if(this->IsInCastingRange())
	{
		//set focus in control and allows strafe
		RangedEnemy->bUseControllerRotationYaw = true;
		Movement->bOrientRotationToMovement = false;
	}
	else{
		RangedEnemy->bUseControllerRotationYaw = false;
		Movement->bOrientRotationToMovement = true;
	}

	FVector PlayerLocation = Target->GetActorLocation();
	const FVector BaseRingOffset(this->GetRingRadius(), 0.0f, 0.0f);
	const FVector RingOffset = BaseRingOffset.RotateAngleAxis(this->GetRingAngleDegrees(), FVector::UpVector);

	FVector DesiredLocation = PlayerLocation + RingOffset;
	DesiredLocation.Z += this->GetHoverHeight();

	const FVector ToTarget = DesiredLocation - RangedEnemy->GetActorLocation();
	const float Distance = ToTarget.Size();

	if(Distance <= this->GetAcceptanceRadius())
	{
		Movement->Velocity = FVector::ZeroVector;
		return;
	}

	//change direction
	const FVector Direction = ToTarget.GetSafeNormal();
	Movement->Velocity = Direction * Movement->MaxFlySpeed;

	FVector StartLocation = RangedEnemy->GetActorLocation();

	FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, PlayerLocation);

	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;

	float DeltaTime = UGameplayStatics::GetWorldDeltaSeconds(this);
	FRotator CurrentRotation = RangedEnemy->GetActorRotation();
	FRotator NewRot = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.0f);

	RangedEnemy->SetActorRotation(NewRot);
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
    GetWorldTimerManager().ClearTimer(RangedMovementTimerHandle);

    if(!HasAuthority() || !GetPawn() || bPausedForPooling)
    {
        return;
    }

    const float Interval = FMath::Max(0.05f, ChaseCheckInterval);
    const float InitialDelay = FMath::FRandRange(0.01f, ChaseCheckInterval);

    GetWorldTimerManager().SetTimer(ChaseTimer, this, &AEnemyRangedAIController::UpdateChase, Interval, true, InitialDelay);
    GetWorldTimerManager().SetTimer(RangedMovementTimerHandle, this, &AEnemyRangedAIController::UpdateMovement, MovementUpdateInterval, true, InitialDelay);
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
    GetWorldTimerManager().ClearTimer(RangedMovementTimerHandle);

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