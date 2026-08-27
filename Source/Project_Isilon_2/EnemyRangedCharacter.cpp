// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyRangedCharacter.h"
#include "EnemyAIController.h"
#include "EnemySpawner.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnemyRangedAIController.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AEnemyRangedCharacter::AEnemyRangedCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	AIControllerClass = AEnemyRangedAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 400.0f, 0.0f);
	Movement->GravityScale = 0.0f;
	Movement->MaxFlySpeed = 350.0f;
	Movement->MaxAcceleration = 500.f;
	Movement->BrakingDecelerationFlying = 350.0f;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
}

// Called when the game starts or when spawned
void AEnemyRangedCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	// const FString AuthorityString = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	// UE_LOG(LogTemp, Warning, TEXT("[RANGED CHARACTER][%s] BeginPlay: %s | Controller: %s"), *AuthorityString, *GetNameSafe(this), *GetNameSafe(GetController()));
}

// Called every frame
void AEnemyRangedCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(!HasAuthority())
	{
		return;
	}

	AEnemyRangedAIController* RangedController = Cast<AEnemyRangedAIController>(GetController());
	if(!IsValid(RangedController))
	{
		return;
	}

	APawn* Target = RangedController->GetCurrentTarget();
	if(!IsValid(Target))
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if(RangedController->IsInAttackRange())
	{
		//set focus in control and allows strafe
		bUseControllerRotationYaw = true;
		Movement->bOrientRotationToMovement = false;
	}
	else{
		bUseControllerRotationYaw = false;
		Movement->bOrientRotationToMovement = true;
	}

	FVector PlayerLocation = Target->GetActorLocation();
	const FVector BaseRingOffset(RangedController->GetRingRadius(), 0.0f, 0.0f);
	const FVector RingOffset = BaseRingOffset.RotateAngleAxis(RangedController->GetRingAngleDegrees(), FVector::UpVector);

	FVector DesiredLocation = PlayerLocation + RingOffset;
	DesiredLocation.Z += RangedController->GetHoverHeight();

	const FVector ToTarget = DesiredLocation - GetActorLocation();
	const float Distance = ToTarget.Size();

	if(Distance <= RangedController->GetAcceptanceRadius())
	{
		return;
	}

	const FVector Direction = ToTarget.GetSafeNormal();
	AddMovementInput(Direction, 1.0f);
}

// Called to bind functionality to input
void AEnemyRangedCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemyRangedCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

//Start Damage Taking Section
float AEnemyRangedCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if(!HasAuthority())
	{
		return 0.0f;
	}

	if(!bPoolActive)
	{
		return 0.0f;
	}

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	RangedEnemyCurrentHealth -= ActualDamage;
	UE_LOG(LogTemp, Warning, TEXT("[SERVER] %s took %.1f damage. Health: %.1f"), *GetName(), ActualDamage, RangedEnemyCurrentHealth);

	if(RangedEnemyCurrentHealth <= 0.0f)
	{
		RangedEnemyCurrentHealth = 0.0f;
		if(IsValid(OwningSpawner.Get()))
		{
			OwningSpawner.Get()->ReturnRangedEnemyToPool(this);
		}
		else
		{
			DeactivateForPool();
		}
	}
	
	return ActualDamage;
}

void AEnemyRangedCharacter::OnRep_RangedEnemyCurrentHealth()
{
	UE_LOG(LogTemp, Warning, TEXT("[CLIENT] %s health Updated: %.1f"), *GetName(), RangedEnemyCurrentHealth);

	//TODO:
	//1. Hit Reaction (Knockback, hit reaction)
}

void AEnemyRangedCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEnemyRangedCharacter, RangedEnemyCurrentHealth);
	DOREPLIFETIME(AEnemyRangedCharacter, bPoolActive);
}

//End Damage Taking Section

//Start Enemy Ranged Object Pooling
void AEnemyRangedCharacter::ApplyPoolState()
{
	SetActorHiddenInGame(!bPoolActive);
	SetActorEnableCollision(bPoolActive);
	SetActorTickEnabled(bPoolActive);

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if(bPoolActive)
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Flying);
	}
	else
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
}

void AEnemyRangedCharacter::ActivateFromPool(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if(!HasAuthority())
	{
		return;
	}

	RangedEnemyCurrentHealth = RangedEnemyBaseHealth;

	SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	Movement->StopMovementImmediately();
	Movement->SetMovementMode(MOVE_Flying);

	bPoolActive = true;

	ApplyPoolState();

	if(AEnemyRangedAIController* EC = Cast<AEnemyRangedAIController>(GetController()))
	{
		EC->ResumeFromPooling();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SERVER] %s has no valid controller to resume from pooling."), *GetNameSafe(this));
	}

	ForceNetUpdate();
}

void AEnemyRangedCharacter::DeactivateForPool()
{
	if(!HasAuthority())
	{
		return;
	}
	
	bPoolActive = false;

	//stop movement
	// Movement->StopMovementImmediately();
	// Movement->DisableMovement();

	if(AEnemyRangedAIController* EnemyController = Cast<AEnemyRangedAIController>(GetController()))
	{
		// EnemyController->StopMovement();
		// EnemyController->ClearFocus(EAIFocusPriority::Gameplay);
		EnemyController->PauseForPooling();
	}

	ApplyPoolState();
	ForceNetUpdate();
}

void AEnemyRangedCharacter::OnRep_PoolActive()
{
	ApplyPoolState();
}
//End Enemy Object Pooling