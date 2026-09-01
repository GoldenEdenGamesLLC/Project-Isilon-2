// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"

#include "EnemySpawner.h"
#include "EnemyAIStats.h"
#include "EnemyAIController.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = true;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 400.0f, 0.0f);

	GetCharacterMovement()->MaxWalkSpeed = 300.0f;
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

//Start Damage Taking Section
float AEnemyCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
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
	MeleeEnemyCurrentHealth = FMath::Clamp(MeleeEnemyCurrentHealth - ActualDamage, 0.0f, MaxHealth);
	UE_LOG(LogTemp, Warning, TEXT("[SERVER] %s took %.1f damage. Health: %.1f"), *GetName(), ActualDamage, MeleeEnemyCurrentHealth);

	if(MeleeEnemyCurrentHealth <= 0.0f)
	{
		if(IsValid(OwningSpawner.Get()))
		{
			MeleeEnemyCurrentHealth = 0.0f;
			
			OwningSpawner.Get()->ReturnMeleeEnemyToPool(this);
		}
	}
	
	return ActualDamage;
}

void AEnemyCharacter::OnRep_MeleeEnemyCurrentHealth()
{
	UE_LOG(LogTemp, Warning, TEXT("[CLIENT] MeleeEnemyCurrentHealth = %s health Updated: %.1f"), *GetName(), MeleeEnemyCurrentHealth);
	
	//TODO:
	//1. Hit Reaction (Knockback, hit reaction)
}

void AEnemyCharacter::OnRep_MaxHealth()
{
	UE_LOG(LogTemp, Warning, TEXT("[CLIENT] MaxHealth = %s health Updated: %.1f"), *GetName(), MaxHealth);
}

void AEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEnemyCharacter, MeleeEnemyCurrentHealth);
	DOREPLIFETIME(AEnemyCharacter, bPoolActive);
	DOREPLIFETIME(AEnemyCharacter, MaxHealth);
}

//End Damage Taking Section

//Begin Enemy Object Pooling
void AEnemyCharacter::ApplyPoolState()
{
	SetActorHiddenInGame(!bPoolActive);
	SetActorEnableCollision(bPoolActive);
	SetActorTickEnabled(bPoolActive);

	if(bPoolActive)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	else
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
}

void AEnemyCharacter::OnRep_PoolActive()
{
	ApplyPoolState();
}

void AEnemyCharacter::ActivateFromPool(const FVector& SpawnLocation, const FRotator& SpawnRotation, const UEnemyAIStats* DifficultyStats, float RuntimeCoefficient)
{
	if(!HasAuthority())
	{
		return;
	}

	// Get health and damage based off of damage coefficient.
	ApplyDifficulty(DifficultyStats, RuntimeCoefficient);

	SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	bPoolActive = true;

	ApplyPoolState();

	if(AEnemyAIController* EC = Cast<AEnemyAIController>(GetController()))
	{
		EC->ResumeFromPooling();
	}

	ForceNetUpdate();
}

void AEnemyCharacter::DeactivateForPool()
{
	if(!HasAuthority())
	{
		return;
	}
	
	bPoolActive = false;

	//stop movement
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	if(AEnemyAIController* EnemyController = Cast<AEnemyAIController>(GetController()))
	{
		EnemyController->StopMovement();
		EnemyController->ClearFocus(EAIFocusPriority::Gameplay);
		EnemyController->PauseForPooling();
	}

	ApplyPoolState();
	ForceNetUpdate();
}
//End Enemy Object Pooling

void AEnemyCharacter::ApplyDifficulty(const UEnemyAIStats* DifficultyStats, float RuntimeCoefficient)
{
	if(!DifficultyStats)
	{
		UE_LOG(LogTemp, Error, TEXT("[DIFFICULTY ERROR]: %s received NULL DifficultyStats! RuntimeCoefficient = %.3f"), *GetName(), RuntimeCoefficient);
		
		MaxHealth = MeleeEnemyBaseHealth;
		MeleeEnemyCurrentHealth = MaxHealth;
		Damage = BaseDamage;

		return;
	}

	const float HealthScaling = FMath::Pow(RuntimeCoefficient, 1.0f);
	const float DamageScaling = FMath::Pow(RuntimeCoefficient, 0.6f);

	const float currentHealthCoefficient = DifficultyStats->GetEnemyHealthCoefficient() * HealthScaling;
	const float currentDamageCoefficient = DifficultyStats->GetEnemyDamageCoefficient() * DamageScaling;
	
	if(SpeedsVariations.Num() > 0 && HealthVariations.Num() > 0)
	{
		RandomIndex = 0;
		RandomIndex = FMath::RandRange(0, SpeedsVariations.Num() - 1);

		GetCharacterMovement()->MaxWalkSpeed = SpeedsVariations[RandomIndex];
		MeleeEnemyBaseHealth = HealthVariations[RandomIndex];
	}
	
	MaxHealth = MeleeEnemyBaseHealth * currentHealthCoefficient;
	MeleeEnemyCurrentHealth = MaxHealth;
	Damage = BaseDamage * currentDamageCoefficient;

	UE_LOG(LogTemp, Warning, TEXT("[DIFFICULTY] %s | RuntimeCoefficient: %.3f | HealthScale: %.3f | DamageScale: %.3f"),*GetName(), RuntimeCoefficient, HealthScaling, DamageScaling);
	UE_LOG(LogTemp, Warning, TEXT("[MELEE RANGED SCALING] CurrentHealthCoefficient = %f, CurrentDamageCoefficient = %f."), currentHealthCoefficient, currentDamageCoefficient);
	UE_LOG(LogTemp, Warning, TEXT("[MELEE RANGED SCALING] CurrentHealth = %f, CurrentDamage = %f."), MeleeEnemyCurrentHealth, Damage);
}