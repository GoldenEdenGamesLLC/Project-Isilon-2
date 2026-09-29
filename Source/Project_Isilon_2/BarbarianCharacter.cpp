// Fill out your copyright notice in the Description page of Project Settings.


#include "BarbarianCharacter.h"

#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "InteractableActor.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"
#include "InputActionValue.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "Blueprint/UserWidget.h"
#include "DrawDebugHelpers.h"

#include "EnemyInterface.h"

//ignores players and focuses on enemies when dashing
#define ECC_Enemy ECC_GameTraceChannel1

// Sets default values
ABarbarianCharacter::ABarbarianCharacter()
{
	bReplicates = true;
	SetReplicateMovement(true);

	//spring arm
	cameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	cameraBoom->SetupAttachment(RootComponent);
	cameraBoom->TargetArmLength = 400.0f;
	cameraBoom->bUsePawnControlRotation = true;
	
	//follow camera
	followCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	followCamera->SetupAttachment(cameraBoom, USpringArmComponent::SocketName);
	followCamera->bUsePawnControlRotation = false;
	
	//Interaction
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(200.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true; //rotate with the mouse
	bUseControllerRotationRoll = false;

	//char movement
	UCharacterMovementComponent* charMoveComp = GetCharacterMovement();
	charMoveComp->bOrientRotationToMovement = true;
	charMoveComp->RotationRate = FRotator(0.0f, 750.0f, 0.0f);
	charMoveComp->MaxWalkSpeed = 500.f; //faster gameplay??
	charMoveComp->MinAnalogWalkSpeed = 20.f;
	charMoveComp->MaxAcceleration = 4000.0f;
	charMoveComp->BrakingDecelerationWalking = 4000.0f;
	charMoveComp->BrakingFrictionFactor = 2.0f;

	//Jump
	charMoveComp->JumpZVelocity = 500.0f;
	charMoveComp->GravityScale = 1.50f;
	JumpMaxCount = 2;

	//Rotation
	charMoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
}

void ABarbarianCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABarbarianCharacter::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ABarbarianCharacter::OnInteractionSphereEndOverlap);
}

void ABarbarianCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	APlayerController* playerController = Cast<APlayerController>(Controller);
	if (!playerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("ERROR: PawnClientRestart PlayerController == null."));
		return;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Success: PawnClientRestart PlayerController is good."));
	}

	if (UEnhancedInputLocalPlayerSubsystem* subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(playerController->GetLocalPlayer()))
	{
		subsystem->AddMappingContext(DefaultMappingContext, 0);
		UE_LOG(LogTemp, Warning, TEXT("PawnClientRestart Enhanced Input Subsystem is good."));
	}

	if(!CrosshairWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CROSSHAIR] CrosshairWidgetClass == null."));
		return;
	}

	if(CrosshairWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CROSSHAIR] CrosshairWidget already exists."));
		return;
	}

	CrosshairWidget = CreateWidget<UUserWidget>(playerController, CrosshairWidgetClass);

	if(!CrosshairWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CROSSHAIR] CrosshairWidget failed."));
		return;
	}

	const bool bAdded = CrosshairWidget->AddToPlayerScreen();
}

void ABarbarianCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//If char has velocity, char follows mouse, else you can look around standing ~still
	FVector Velocity = GetVelocity();
	const float HorizontalSpeed = FVector(Velocity.X, Velocity.Y, 0.0f).Size();

	if(HorizontalSpeed > 5.0f)
	{
		bUseControllerRotationYaw = true;
	}
	else
	{
		bUseControllerRotationYaw = false;
	}

	if(bIsDashing)
	{
		HandleDashDamage();
	}
}

// Called to bind functionality to input
void ABarbarianCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ABarbarianCharacter::Move);
		EnhancedInputComponent->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ABarbarianCharacter::Look);
		EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Started, this, &ABarbarianCharacter::JumpPressed);
		EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ABarbarianCharacter::StopJumping);
		EnhancedInputComponent->BindAction(IA_Dash, ETriggerEvent::Started, this, &ABarbarianCharacter::DashPressed);
		EnhancedInputComponent->BindAction(IA_Interact, ETriggerEvent::Started, this, &ABarbarianCharacter::InteractWithObject);
		EnhancedInputComponent->BindAction(IA_BasicAttack, ETriggerEvent::Started, this, &ABarbarianCharacter::BasicAttackPressed);
		EnhancedInputComponent->BindAction(IA_Massacre, ETriggerEvent::Started, this, &ABarbarianCharacter::MassacrePressed);
		EnhancedInputComponent->BindAction(IA_Ultimate, ETriggerEvent::Started, this, &ABarbarianCharacter::ChainsOfRagePressed);
	}
}

void ABarbarianCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABarbarianCharacter, CurrentHealth);
}

void ABarbarianCharacter::Move(const FInputActionValue& val)
{
	if(!Controller)
	{
		return;
	}

	const FVector2D movementVec = val.Get<FVector2D>();

	const FRotator ControlRot = GetController()->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRot.Yaw, 0.0f);

	const FVector ForwardVec = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightVec = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardVec, movementVec.Y);
	AddMovementInput(RightVec, movementVec.X);
}

void ABarbarianCharacter::Look(const FInputActionValue& val)
{
	const FVector2D value = val.Get<FVector2D>();

	AddControllerYawInput(value.X);
	AddControllerPitchInput(value.Y);
}

//BEGIN JUMP
void ABarbarianCharacter::JumpPressed()
{
	if (CanJump() && JumpTotals < JumpMaxCount)
	{
		JumpTotals++;
		Jump();
	}
}

bool ABarbarianCharacter::CanJumpInternal_Implementation() const
{
	return bCanJumpCooldown && Super::CanJumpInternal_Implementation();
}

void ABarbarianCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	if(JumpTotals <= 1)
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if(!Movement)
	{
		return;
	}

	//Change directions when jumping
	FVector InputDirection = GetLastMovementInputVector();
	InputDirection.Z = 0.0f;

	if(InputDirection.IsNearlyZero())
	{
		//keep momentum since no directional input
		return;
	}

	InputDirection.Normalize();

	FVector HorizontalVelocity = Movement->Velocity;
	HorizontalVelocity.Z = 0.0f;

	const float CurrentHorizontalSpeed = HorizontalVelocity.Size();
	// const float CurrentHorizontalSpeed = Movement->MaxWalkSpeed;

	const FVector NewHorizontalVelocity = InputDirection * CurrentHorizontalSpeed;

	Movement->Velocity.X = NewHorizontalVelocity.X;
	Movement->Velocity.Y = NewHorizontalVelocity.Y;
}

void ABarbarianCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	GetWorldTimerManager().SetTimer(JumpCooldownTimerHandle, this, &ABarbarianCharacter::ResetJumpCooldown, JumpCooldownTime, false);
}

void ABarbarianCharacter::ResetJumpCooldown()
{
	JumpTotals = 0;
	bCanJumpCooldown = true;
}
//END JUMP

//START DASH
void ABarbarianCharacter::DashPressed()
{
	if(!bCanDash || !followCamera)
	{
		return;
	}

	FVector DashDirection = followCamera->GetForwardVector();
	DashDirection.Normalize();
	bCanDash = false;
	bIsDashing = true;

	EnterGhostMode();
	if(!HasAuthority())
	{
		ServerEnterGhostMode();
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	
	bDashStartedOnGround = Movement->IsMovingOnGround();
	//UE_LOG(LogTemp, Warning, TEXT("bDashStartedOnGrounnd = %s"), (bDashStartedOnGround ? TEXT("True") : TEXT("False")));
	Movement->SetMovementMode(MOVE_Flying);
	if(!bDashStartedOnGround)
	{
		Movement->GravityScale = 1.5f;
	}
	Movement->Velocity = DashDirection * DashStrength;

	// GetWorldTimerManager().SetTimer(DashDurationTimerHandle, this, &ABarbarianCharacter::HandleDashDamage, DashDuration, false);
	GetWorldTimerManager().SetTimer(DashDurationTimerHandle, this, &ABarbarianCharacter::StopDash, DashDuration, false);
	GetWorldTimerManager().SetTimer(DashCooldownTimerHandle, this, &ABarbarianCharacter::ResetDashCooldown, DashCooldownTime, false);
}

void ABarbarianCharacter::ResetDashCooldown()
{
	bCanDash = true;
}

void ABarbarianCharacter::StopDash()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	
	ExitGhostMode();
	if(!HasAuthority())
	{
		ServerExitGhostMode();
	}

	if(Movement->IsMovingOnGround())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	else
	{
		Movement->SetMovementMode(MOVE_Falling);
	}
	
	Movement->Velocity = FVector::ZeroVector;
	bDashStartedOnGround = false;
	UE_LOG(LogTemp, Warning, TEXT("JumpTotals = %d"), JumpTotals)
	
	bIsDashing = false;
	damagedActorsByDash.Reset();
}

void ABarbarianCharacter::HandleDashDamage()
{
	if(!HasAuthority())
	{
		return;
	}

	const FRotator AimRotation = followCamera->GetComponentRotation();
	const FVector DashDirection = AimRotation.Vector().GetSafeNormal();
	const FRotator CharacterRotation(0.0f, AimRotation.Yaw, 0.0f);

	SetActorRotation(CharacterRotation);

	FVector Start = GetActorLocation();
	FVector DashStart = Start + (DashDirection * DashDamageStartRadius);
	FVector End = Start + (DashDirection * DashDamageEndRadius);

	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionShape DashCapsuleSweep = FCollisionShape::MakeCapsule(DashDamageStartRadius, DashCapsuleHalfHeight);

	bool bHit = GetWorld()->SweepMultiByChannel(HitResults, DashStart, End, FQuat::Identity, ECC_Pawn, DashCapsuleSweep, QueryParams);

	if(bHit)
	{
		for(auto& hit : HitResults)
		{
			AActor* HitActor = hit.GetActor();
			if(HitActor && !damagedActorsByDash.Contains(HitActor))
			{
				UE_LOG(LogTemp, Warning, TEXT("[SERVER] Dash Damage: %s"), *HitActor->GetName());
				damagedActorsByDash.Add(HitActor);

				UGameplayStatics::ApplyDamage(HitActor, DashDamage, GetController(), this, UDamageType::StaticClass());
			}
		}
	}

	MulticastDashHandlerFX(DashStart, End, bHit);
}

//pass through enemies
void ABarbarianCharacter::ServerEnterGhostMode_Implementation()
{
	EnterGhostMode();
}

void ABarbarianCharacter::EnterGhostMode()
{
	if(UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		PreviousPawnCollisionResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

		const ECollisionResponse response = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
		UE_LOG(LogTemp, Warning, TEXT("[%s] ENTER GHOST | %s | Pawn=%d"), HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), *GetName(), (int32)Capsule->GetCollisionResponseToChannel(ECC_Pawn));
	}
}

void ABarbarianCharacter::ServerExitGhostMode_Implementation()
{
	ExitGhostMode();
}

void ABarbarianCharacter::ExitGhostMode()
{
	if(UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, PreviousPawnCollisionResponse);
	}
}

void ABarbarianCharacter::MulticastDashHandlerFX_Implementation(FVector Start, FVector End, bool bHit)
{
	DrawDebugCapsule(GetWorld(), Start, DashCapsuleHalfHeight, DashDamageStartRadius, FQuat::Identity, FColor::Yellow, false, 1.0f);

	DrawDebugCapsule(GetWorld(), End, DashCapsuleHalfHeight, DashDamageEndRadius, FQuat::Identity, bHit ? FColor::Red : FColor::Green, false, 1.0f);
}
//END DASH

//BEGIN BASIC ATTACK
void ABarbarianCharacter::BasicAttackPressed()
{
	if(!bCanBasicAttack)
	{
		return;
	}

	const FRotator AimRotation = followCamera->GetComponentRotation();

	FRotator AttackRotation(0.0f, AimRotation.Yaw, 0.0f);
	SetActorRotation(AttackRotation);

	if(HasAuthority())
	{
		PerformBasicAttack(AimRotation);
		return;
	}

	bCanBasicAttack = false;
	GetWorldTimerManager().SetTimer(BasicAttackCooldownTimerHandle, this, &ABarbarianCharacter::BasicAttackCompleted, BasicAttackSpeed, false);

	ServerBasicAttack(AimRotation);
}

void ABarbarianCharacter::BasicAttackCompleted()
{
	ResetBasicAttackCooldown();
}

void ABarbarianCharacter::ResetBasicAttackCooldown()
{
	bCanBasicAttack = true;
}

void ABarbarianCharacter::ServerBasicAttack_Implementation(FRotator AimRotation)
{
	if(!bCanBasicAttack)
	{
		return;
	}

	PerformBasicAttack(AimRotation);
}

void ABarbarianCharacter::PerformBasicAttack(FRotator AimRotation)
{
	if(!HasAuthority())
	{
		return;
	}

	if(!bCanBasicAttack)
	{
		return;
	}

	bCanBasicAttack = false;
	
	const FVector AttackDirection = AimRotation.Vector().GetSafeNormal();
	const FRotator CharacterRotation(0.0f, AimRotation.Yaw, 0.0f);

	SetActorRotation(CharacterRotation);
	
	//specifically for the barbarian
	FVector Start = GetActorLocation();
	FVector AttackStart = Start + (AttackDirection * BasicAttackStartDistance);
	FVector End = Start + (AttackDirection * BasicAttackEndDistance);

	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	//upgrade makes capsule
	FCollisionShape AttackSweep = FCollisionShape::MakeCapsule(BasicAttackRadius, BasicAttackHalfHeight);

	bool bHit = GetWorld()->SweepMultiByChannel(HitResults, AttackStart, End, FQuat::Identity, ECC_Pawn, AttackSweep, QueryParams);

	TSet<AActor*> damagedActors;
	if(bHit)
	{
		for(auto& Hit : HitResults)
		{
			//deal damage
			AActor* HitActor = Hit.GetActor();
			if(HitActor)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SERVER] Basic Attack Hit: %s"), *HitActor->GetName());
				damagedActors.Add(HitActor);
				
				UGameplayStatics::ApplyDamage(HitActor, BasicAttackDamage, GetController(), this, UDamageType::StaticClass());
			}
		}
	}

	MulticastBasicAttackFX(AttackStart, End, bHit);

	GetWorldTimerManager().SetTimer(BasicAttackCooldownTimerHandle, this, &ABarbarianCharacter::BasicAttackCompleted, BasicAttackSpeed, false);
}

void ABarbarianCharacter::MulticastBasicAttackFX_Implementation(FVector AttackStart, FVector End, bool bHit)
{
	DrawDebugCapsule(GetWorld(), AttackStart, BasicAttackHalfHeight, BasicAttackRadius, FQuat::Identity, FColor::Yellow, false, 1.0f);

	DrawDebugCapsule(GetWorld(), End, BasicAttackHalfHeight, BasicAttackRadius, FQuat::Identity, bHit ? FColor::Red : FColor::Green, false, 1.0f);
}
//END BASIC ATTACK

// START INTERACTION
void ABarbarianCharacter::InteractWithObject(const FInputActionValue& val)
{
	if(!NearbyInteractable)
	{
		return;
	}

	if(HasAuthority())
	{
		NearbyInteractable->Interact(this);
	}
	else
	{
		ServerInteract(NearbyInteractable);
	}
}

void ABarbarianCharacter::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(!OtherActor || OtherActor == this)
	{
		return;
	}

	AInteractableActor* Interactable = Cast<AInteractableActor>(OtherActor);

	if(Interactable)
	{
		NearbyInteractable = Interactable;

		UE_LOG(LogTemp, Warning, TEXT("Inside range to press [E] to Interact with %s."), *Interactable->GetName());

		FSphereInteractionParams Params;
		
		Params.OverlappedComponent = OverlappedComponent;
		Params.OtherActor = OtherActor;
		Params.OtherComp = OtherComp;
		Params.OtherBodyIndex = OtherBodyIndex;
		Params.bFromSweep = bFromSweep;
		Params.SweepResult = SweepResult;
		
		HandleInteractionOverlap(Params);
		//ShowInteractionPrompt();
	}
}

void ABarbarianCharacter::OnInteractionSphereEndOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
	if(OtherActor == NearbyInteractable)
	{
		NearbyInteractable = nullptr;

		UE_LOG(LogTemp, Warning, TEXT("Left interaction range."));

		//HideInteractionPrompt();
	}
}

void ABarbarianCharacter::HandleInteractionOverlap(const FSphereInteractionParams& Params)
{
	
}

void ABarbarianCharacter::ServerInteract_Implementation(AInteractableActor* Interactable)
{
	if(!Interactable)
	{
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), Interactable->GetActorLocation());

	if(Distance > 250.0f)
	{
		return;
	}

	Interactable->Interact(this);
}

// START DEFENSE
void ABarbarianCharacter::OnRep_CurrentHealth()
{
	UE_LOG(LogTemp, Warning, TEXT("[CLIENT] Player Character Current Health - Barbarian = %s health Updated: %.1f"), *GetName(), CurrentHealth);	
}

float ABarbarianCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if(!HasAuthority())
	{
		return 0.0f;
	}

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
	UE_LOG(LogTemp, Warning, TEXT("[SERVER] %s took %.1f damage. Health: %.1f"), *GetName(), ActualDamage, CurrentHealth);

	if(CurrentHealth <= 0.0f)
	{
		//die
	}
	
	return ActualDamage;
}
// END DEFENSE

// BEGIN Massacre
//TODO:
//1. Fix Character snapping for rotation
//2. Massacre not resetting
void ABarbarianCharacter::MassacrePressed()
{
	if(!bCanMassacre)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[MASSACRE DEBUG] Massacre Pressed - MassacreAttackCount = %d"), MassacreAttackCount);

	if(HasAuthority())
	{
		PerformMassacre();
		return;
	}

	GetWorldTimerManager().SetTimer(MassacreCooldownTimerHandle, this, &ABarbarianCharacter::ResetMassacreCooldown, MassacreCooldown, false);

	ServerMassacre();
}

void ABarbarianCharacter::ServerMassacre_Implementation()
{
	if(!bCanMassacre)
	{
		return;
	}

	PerformMassacre();
}

void ABarbarianCharacter::PerformMassacre()
{
	if(!HasAuthority())
	{
		return;
	}

	if(bCanMassacre)
	{
		bCanMassacre = false;
	}

	if(MaxMassacreAttackCount > MassacreAttackCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MASSACRE DEBUG] Attack #%d"), MassacreAttackCount);
		MassacreAttackCount++;
		
		const FRotator AimRotation = followCamera->GetComponentRotation();
		const FVector AttackDirection = AimRotation.Vector().GetSafeNormal();

		FVector StartingLocation = GetActorLocation();
		FVector MassacreStart = StartingLocation + (AttackDirection * MassacreStartDistance);
		FVector MassacreEnd = StartingLocation + (AttackDirection * MassacreEndDistance);

		TArray<FHitResult> HitResults;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		//upgrade makes capsule
		FCollisionShape MassacreSweep = FCollisionShape::MakeCapsule(MassacreRadius, MassacreHalfHeight);

		bool bHit = GetWorld()->SweepMultiByChannel(HitResults, MassacreStart, MassacreEnd, FQuat::Identity, ECC_Pawn, MassacreSweep, QueryParams);

		TSet<AActor*> damagedActors;
		if(bHit)
		{
			for(auto& Hit : HitResults)
			{
				//deal damage
				AActor* HitActor = Hit.GetActor();
				if(HitActor)
				{
					UE_LOG(LogTemp, Warning, TEXT("[MASSACRE SERVER] Massacre Attack Hit: %s"), *HitActor->GetName());
					damagedActors.Add(HitActor);
					
					UGameplayStatics::ApplyDamage(HitActor, MassacreAttackDamage, GetController(), this, UDamageType::StaticClass());
				}
			}
		}
		
		MulticastMassacreFX(MassacreStart, MassacreEnd, bHit);

		if(MaxMassacreAttackCount > MassacreAttackCount)
		{
			if(!GetWorldTimerManager().IsTimerActive(MassacreAttackTimerHandle))
			{
				//must use lambda to pass 
				//GetWorldTimerManager().SetTimer(MassacreAttackTimerHandle, [this, AimRotation]() { this->PerformMassacre(AimRotation); }, MassacreAttackSpeed, false);
				GetWorldTimerManager().SetTimer(MassacreAttackTimerHandle, this, &ABarbarianCharacter::PerformMassacre, MassacreAttackSpeed, true);
			}
		}
	}

	//Reset once attacks >= 5
	if(MaxMassacreAttackCount <= MassacreAttackCount)
	{
		if(!GetWorldTimerManager().IsTimerActive(MassacreCooldownTimerHandle))
		{
			GetWorldTimerManager().SetTimer(MassacreCooldownTimerHandle, this, &ABarbarianCharacter::ResetMassacreCooldown, MassacreCooldown, false);
		}
	}
}

void ABarbarianCharacter::ResetMassacreCooldown()
{
	bCanMassacre = true;
	MassacreAttackCount = 0;

	GetWorldTimerManager().ClearTimer(MassacreCooldownTimerHandle);
	GetWorldTimerManager().ClearTimer(MassacreAttackTimerHandle);
}

void ABarbarianCharacter::MulticastMassacreFX_Implementation(FVector Start, FVector End, bool bHit)
{
	DrawDebugCapsule(GetWorld(), Start, MassacreHalfHeight, MassacreRadius, FQuat::Identity, FColor::Yellow, false, 1.0f);

	DrawDebugCapsule(GetWorld(), End, MassacreHalfHeight, MassacreRadius, FQuat::Identity, bHit ? FColor::Red : FColor::Green, false, 1.0f);
}
// END Massacre

// BEGIN ULTIMATE
void ABarbarianCharacter::ChainsOfRagePressed()
{
	if(!bCanUltimate)
	{
		return;
	}

	if(!HasAuthority())
	{
		return;
	}

	ChainsOfRageThrowAxe();
}

void ABarbarianCharacter::ChainsOfRageThrowAxe()
{
	FVector BarbarianLocation = GetActorLocation();

	const FVector TraceStart = BarbarianLocation;
	const FVector AimDirection = followCamera->GetForwardVector().GetSafeNormal();
	const FVector TraceEnd = TraceStart + (AimDirection * 1000.0f);
	
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, Params);

	FVector ImpactPoint;

	if(bHit)
	{
		ImpactPoint = HitResult.ImpactPoint;
	}
	else
	{
		ImpactPoint = TraceEnd;
	}

	DrawDebugLine(GetWorld(), TraceStart, ImpactPoint, FColor::White, false, 2.0f);
	DrawDebugSphere(GetWorld(), ImpactPoint, 20.0f, 12, FColor::Blue, false, 2.0f);

	PostImpactChainsOfRage(ImpactPoint);
}

void ABarbarianCharacter::PostImpactChainsOfRage(const FVector& ImpactPoint)
{
	if(!HasAuthority())
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;

	FCollisionShape AxeImpactSphere = FCollisionShape::MakeSphere(CoRAxeLandingRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bFoundEnemies = GetWorld()->OverlapMultiByObjectType(OverlapResults, ImpactPoint, FQuat::Identity, ObjectParams, AxeImpactSphere, QueryParams);
	if(bFoundEnemies)
	{
		for(const auto& OverlapHits : OverlapResults)
		{
			AActor* HitActor = OverlapHits.GetActor();

			if(!IsValid(HitActor))
			{
				continue;
			}

			if(!HitActor->Implements<UEnemyInterface>())
			{
				continue;
			}

			if(!IEnemyInterface::Execute_IsEnemyActive(HitActor))
			{
				continue;
			}

			AActor* EnemyActor = IEnemyInterface::Execute_GetEnemyActor(HitActor);
			if(!IsValid(EnemyActor))
			{
				continue;
			}

			ChainsOfRageTargets.AddUnique(EnemyActor);
		}
	}
}

void ABarbarianCharacter::UpdateChainsOfRageTethers()
{

}

void ABarbarianCharacter::ResetUltimateThrowCooldown()
{

}

void ABarbarianCharacter::MulticastUltimateThrowFX_Implementation(FVector Start, FVector End, bool bHit)
{

}
// END ULTIMATE