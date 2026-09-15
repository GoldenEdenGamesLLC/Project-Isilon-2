// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyRangedCharacter.h"
#include "EnemyRangedAIController.h"
#include "EnemySpawner.h"
#include "EnemyAIStats.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnemyRangedAIController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AEnemyRangedCharacter::AEnemyRangedCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

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
	Movement->MaxFlySpeed = 200.0f;
	Movement->MaxAcceleration = 300.f;
	Movement->BrakingDecelerationFlying = 350.0f;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
}

// Called when the game starts or when spawned
void AEnemyRangedCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}

// Called every frame
void AEnemyRangedCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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

void AEnemyRangedCharacter::UpdateRangedMovement()
{
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
	if(RangedController->IsInCastingRange())
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
		Movement->Velocity = FVector::ZeroVector;
		return;
	}

	//change direction
	const FVector Direction = ToTarget.GetSafeNormal();
	Movement->Velocity = Direction * Movement->MaxFlySpeed;

	FVector StartLocation = GetActorLocation();

	FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, PlayerLocation);

	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;

	float DeltaTime = UGameplayStatics::GetWorldDeltaSeconds(this);
	FRotator CurrentRotation = GetActorRotation();
	FRotator NewRot = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.0f);

	SetActorRotation(NewRot);
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

	RangedEnemyCurrentHealth = FMath::Clamp(RangedEnemyCurrentHealth - ActualDamage, 0.0f, MaxHealth);

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
	UE_LOG(LogTemp, Warning, TEXT("[CLIENT] RangedEnemyCurrentHealth =  %s health Updated: %.1f"), *GetName(), RangedEnemyCurrentHealth);

	//TODO:
	//1. Hit Reaction (Knockback, hit reaction)
}

void AEnemyRangedCharacter::OnRep_MaxHealth()
{
	UE_LOG(LogTemp, Warning, TEXT("[CLIENT] MaxHealth = %s health Updated: %.1f"), *GetName(), MaxHealth);
}

void AEnemyRangedCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEnemyRangedCharacter, RangedEnemyCurrentHealth);
	DOREPLIFETIME(AEnemyRangedCharacter, bPoolActive);
	DOREPLIFETIME(AEnemyRangedCharacter, MaxHealth);
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

void AEnemyRangedCharacter::ActivateFromPool(const FVector& SpawnLocation, const FRotator& SpawnRotation, const UEnemyAIStats* DifficultyStats, float RuntimeCoefficient)
{
	if(!HasAuthority())
	{
		return;
	}

	// Get health based off of damage coefficient.
	ApplyDifficulty(DifficultyStats, RuntimeCoefficient);

	SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

	ResetBasicCast();

	bPoolActive = true;

	ApplyPoolState();

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	Movement->StopMovementImmediately();
	Movement->SetMovementMode(MOVE_Flying);

	if(AEnemyRangedAIController* EC = Cast<AEnemyRangedAIController>(GetController()))
	{
		EC->ResumeFromPooling();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SERVER] %s has no valid controller to resume from pooling."), *GetNameSafe(this));
	}
	
	GetWorldTimerManager().ClearTimer(RangedMovementTimerHandle);
	GetWorldTimerManager().SetTimer(RangedMovementTimerHandle, this, &AEnemyRangedCharacter::UpdateRangedMovement, MovementUpdateInterval, true);

	ForceNetUpdate();
}

void AEnemyRangedCharacter::DeactivateForPool()
{
	if(!HasAuthority())
	{
		return;
	}
	
	bPoolActive = false;

	ResetBasicCast();
	GetWorldTimerManager().ClearTimer(RangedMovementTimerHandle);

	GetCharacterMovement()->StopMovementImmediately();

	if(AEnemyRangedAIController* EnemyController = Cast<AEnemyRangedAIController>(GetController()))
	{
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

void AEnemyRangedCharacter::ApplyDifficulty(const UEnemyAIStats* DifficultyStats, float RuntimeCoefficient)
{
	if(!DifficultyStats)
	{
		UE_LOG(LogTemp, Error, TEXT("[DIFFICULTY ERROR]: %s received NULL DifficultyStats! RuntimeCoefficient = %.3f"), *GetName(), RuntimeCoefficient);

		MaxHealth = RangedEnemyBaseHealth;
		RangedEnemyCurrentHealth = MaxHealth;
		Damage = BaseDamage;

		return;
	}

	const float HealthScaling = FMath::Pow(RuntimeCoefficient, 1.0f);
	const float DamageScaling = FMath::Pow(RuntimeCoefficient, 0.6f);

	const float currentHealthCoefficient = DifficultyStats->GetEnemyHealthCoefficient() * HealthScaling;
	const float currentDamageCoefficient = DifficultyStats->GetEnemyDamageCoefficient() * DamageScaling;
	
	MaxHealth = RangedEnemyBaseHealth * currentHealthCoefficient;
	RangedEnemyCurrentHealth = MaxHealth;
	Damage = BaseDamage * currentDamageCoefficient;
	
	// UE_LOG(LogTemp, Warning, TEXT("[DIFFICULTY] %s | RuntimeCoefficient: %.3f | HealthScale: %.3f | DamageScale: %.3f"),*GetName(), RuntimeCoefficient, HealthScaling, DamageScaling);
	// UE_LOG(LogTemp, Warning, TEXT("[ENEMY RANGED SCALING] CurrentHealthCoefficient = %f, CurrentDamageCoefficient = %f."), currentHealthCoefficient, currentDamageCoefficient);
	// UE_LOG(LogTemp, Warning, TEXT("[ENEMY RANGED SCALING] CurrentHealth = %f, CurrentDamage = %f."), RangedEnemyCurrentHealth, Damage);
}
//RANGED ENEMY ATTACK START

//Start Attack
void AEnemyRangedCharacter::PerformCast(AActor* Target)
{
	if(!HasAuthority())
	{
		return;
	}

	if(!IsValid(Target))
	{
		return;
	}

	const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation());

	if(DistanceSquared > FMath::Square(GetCastExitDistance()))
	{
		return;
	}

	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	UE_LOG(LogTemp, Warning, TEXT("[RANGED ENEMY ATTACK] %s attacked %s for %.1f damage."), *GetName(), *Target->GetName(), Damage);
	UGameplayStatics::ApplyDamage(Target, Damage, GetController(), this, UDamageType::StaticClass());
	MulticastAttackVFX(Start, End, true);
}

void AEnemyRangedCharacter::DrawCastTelegraph()
{
	if(!bTelegraphActive)
	{
		return;
	}

	AActor* Target = TelegraphTarget.Get();

	if(!IsValid(Target))
	{
		GetWorldTimerManager().ClearTimer(TelegraphTimerHandle);

		bTelegraphActive = false;
		TelegraphTarget.Reset();
		return;
	}

	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	const float Elapsed = GetWorld()->GetTimeSeconds() - TelegraphStartTime;

	const float Alpha = FMath::Clamp(Elapsed / CastingTime, 0.0f, 1.0f);

	const float LineThickness = FMath::Lerp(2.0f, 10.0f, Alpha);
	const float SphereRadius = FMath::Lerp(10.0f, 30.0f, Alpha);

	DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, TelegraphUpdateInterval * 1.5f, 0, LineThickness);

	DrawDebugSphere(GetWorld(), End, SphereRadius, 16, FColor::Yellow, false, TelegraphUpdateInterval * 1.5f);
}

void AEnemyRangedCharacter::MulticastStartCastTelegraphVFX_Implementation(AActor* Target)
{
	if(!IsValid(Target))
	{
		return;
	}

	TelegraphTarget = Target;
	bTelegraphActive = true;

	TelegraphStartTime = GetWorld()->GetTimeSeconds();

	GetWorldTimerManager().ClearTimer(TelegraphTimerHandle);
	GetWorldTimerManager().SetTimer(TelegraphTimerHandle, this, &AEnemyRangedCharacter::DrawCastTelegraph, TelegraphUpdateInterval, true);
}

void AEnemyRangedCharacter::MulticastStopCastTelegraphVFX_Implementation()
{
	bTelegraphActive = false;

	GetWorldTimerManager().ClearTimer(TelegraphTimerHandle);

	TelegraphTarget.Reset();
}

void AEnemyRangedCharacter::MulticastAttackVFX_Implementation(FVector Start, FVector End, bool bHit)
{
	const FColor LineColor = bHit ? FColor::Red : FColor::Green;
	DrawDebugLine(GetWorld(), Start, End, LineColor, false, 0.15f, 0, 8.0f);

	DrawDebugSphere(GetWorld(), End, 20.0f, 12, LineColor, false, 0.15f, 0, 3.0f);
}

void AEnemyRangedCharacter::ResetCastCooldown()
{
	bCanCast = true;
}

void AEnemyRangedCharacter::TryCast(AActor* Target)
{
	if(!HasAuthority())
	{
		return;
	}

	if(!bPoolActive)
	{
		return;
	}

	if(!bCanCast)
	{
		return;
	}

	if(!IsValid(Target))
	{
		return;
	}

	const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation());

	if(DistanceSquared > FMath::Square(GetCastExitDistance()))
	{
		return;
	}

	BeginCast(Target);
}

void AEnemyRangedCharacter::BeginCast(AActor* Target)
{
	if(!HasAuthority() || !IsValid(Target))
	{
		return;
	}

	
	bCanCast = false;
	bIsCasting = true;
	
	CastingTarget = Target;
	
	MulticastStartCastTelegraphVFX(Target);

	GetWorldTimerManager().SetTimer(RangedCastingTimerHandle, this, &AEnemyRangedCharacter::CompleteCast, CastingTime, false);
}

void AEnemyRangedCharacter::CompleteCast()
{
	if(!HasAuthority())
	{
		return;
	}

	bIsCasting = false;

	AActor* Target = CastingTarget.Get();

	if(!IsValid(Target))
	{
		CancelCast();
		return;
	}

	const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation());

	if(DistanceSquared > FMath::Square(GetCastExitDistance()))
	{
		CancelCast();
		return;
	}

	MulticastStopCastTelegraphVFX();

	PerformCast(Target);
	CastingTarget.Reset();

	GetWorldTimerManager().SetTimer(RangedCastCooldownTimerHandle, this, &AEnemyRangedCharacter::ResetCastCooldown, CastingCooldown, false);
}

void AEnemyRangedCharacter::CancelCast()
{
	if(!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(RangedCastingTimerHandle);

	MulticastStopCastTelegraphVFX();
	CastingTarget.Reset();

	bIsCasting = false;
	bCanCast = true;
}

void AEnemyRangedCharacter::ResetBasicCast()
{
	GetWorldTimerManager().ClearTimer(RangedCastCooldownTimerHandle);
	GetWorldTimerManager().ClearTimer(RangedCastingTimerHandle);
	GetWorldTimerManager().ClearTimer(TelegraphTimerHandle);

	if(HasAuthority())
	{
		MulticastStopCastTelegraphVFX();
	}

	CastingTarget.Reset();
	TelegraphTarget.Reset();

	bIsCasting = false;
	bCanCast = true;
	bTelegraphActive = false;
}
//End Enemy Ranged Attack