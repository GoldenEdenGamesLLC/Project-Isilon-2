// Fill out your copyright notice in the Description page of Project Settings.


#include "WizardCharacter.h"

#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "InteractableActor.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"
#include "InputActionValue.h"
#include "AIController.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "Blueprint/UserWidget.h"
#include "DrawDebugHelpers.h"

// Sets default values
AWizardCharacter::AWizardCharacter()
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

// Called when the game starts or when spawned
void AWizardCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AWizardCharacter::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AWizardCharacter::OnInteractionSphereEndOverlap);
}

// Called every frame
void AWizardCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
}

// Called to bind functionality to input
void AWizardCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AWizardCharacter::Move);
		EnhancedInputComponent->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AWizardCharacter::Look);
		EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Started, this, &AWizardCharacter::JumpPressed);
		EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AWizardCharacter::StopJumping);
		EnhancedInputComponent->BindAction(IA_Interact, ETriggerEvent::Started, this, &AWizardCharacter::InteractWithObject);
		EnhancedInputComponent->BindAction(IA_BasicAttack, ETriggerEvent::Started, this, &AWizardCharacter::BasicAttackPressed);
	}
}

void AWizardCharacter::PawnClientRestart()
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

void AWizardCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWizardCharacter, CurrentHealth);
}

void AWizardCharacter::Move(const FInputActionValue& val)
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

void AWizardCharacter::Look(const FInputActionValue& val)
{
	const FVector2D value = val.Get<FVector2D>();

	AddControllerYawInput(value.X);
	AddControllerPitchInput(value.Y);
}

//BEGIN JUMP
void AWizardCharacter::JumpPressed()
{
	if (CanJump() && JumpTotals < JumpMaxCount)
	{
		JumpTotals++;
		Jump();
	}
}

bool AWizardCharacter::CanJumpInternal_Implementation() const
{
	return bCanJumpCooldown && Super::CanJumpInternal_Implementation();
}

void AWizardCharacter::OnJumped_Implementation()
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

void AWizardCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	GetWorldTimerManager().SetTimer(JumpCooldownTimerHandle, this, &AWizardCharacter::ResetJumpCooldown, JumpCooldownTime, false);
}

void AWizardCharacter::ResetJumpCooldown()
{
	JumpTotals = 0;
	bCanJumpCooldown = true;
}

//END JUMP

// START INTERACTION
void AWizardCharacter::InteractWithObject(const FInputActionValue& val)
{
	if(!NearbyInteractable)
	{
		return;
	}

	if(HasAuthority())
	{
		ServerInteract_Implementation(NearbyInteractable);
	}
	else
	{
		ServerInteract(NearbyInteractable);
	}
}

void AWizardCharacter::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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

void AWizardCharacter::OnInteractionSphereEndOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
	if(OtherActor == NearbyInteractable)
	{
		NearbyInteractable = nullptr;

		UE_LOG(LogTemp, Warning, TEXT("Left interaction range."));

		//HideInteractionPrompt();
	}
}

void AWizardCharacter::HandleInteractionOverlap(const FSphereInteractionParams& Params)
{
	
}

void AWizardCharacter::ServerInteract_Implementation(AInteractableActor* Interactable)
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
//END INTERACTION

// START DEFENSE
void AWizardCharacter::OnRep_CurrentHealth()
{
	UE_LOG(LogTemp, Warning, TEXT("[CLIENT] Player Character Current Health - Barbarian = %s health Updated: %.1f"), *GetName(), CurrentHealth);	
}

float AWizardCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if(!HasAuthority())
	{
		return 0.0f;
	}

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
	//UE_LOG(LogTemp, Warning, TEXT("[SERVER] %s took %.1f damage. Health: %.1f"), *GetName(), ActualDamage, CurrentHealth);

	if(CurrentHealth <= 0.0f)
	{
		//die
	}
	
	return ActualDamage;
}
// END DEFENSE

// left click is single chaos elemental damage type
// Basic Attack Start
void AWizardCharacter::BasicAttackPressed()
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

	GetWorldTimerManager().SetTimer(BasicAttackCooldownTimerHandle, this, &AWizardCharacter::BasicAttackCompleted, BasicAttackSpeed, false);

	ServerBasicAttack(AimRotation);
}

void AWizardCharacter::ServerBasicAttack_Implementation(FRotator AimRotation)
{
	if(!bCanBasicAttack)
	{
		return;
	}

	PerformBasicAttack(AimRotation);
}

void AWizardCharacter::PerformBasicAttack(FRotator AimRotation)
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

	FVector Start = GetActorLocation();
	FVector AttackStart = Start + (AttackDirection * BasicAttackStartDistance);
	FVector End = Start + (AttackDirection * BasicAttackEndDistance);

	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionShape AttackSweep = FCollisionShape::MakeCapsule(BasicAttackRadius, BasicAttackHalfHeight);

	bool bHit = GetWorld()->SweepMultiByChannel(HitResults, AttackStart, End, FQuat::Identity, ECC_Pawn, AttackSweep, QueryParams);

	TSet<AActor*> damagedActors;
	if(bHit)
	{
		for(auto& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if(HitActor)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SERVER] Basic Attack Hit: %s"), *HitActor->GetName());
				damagedActors.Add(HitActor);

				UGameplayStatics::ApplyDamage(HitActor, BasicAttackDamage /*Change to be the wizard spell calc*/, GetController(), this, UDamageType::StaticClass() /*Change to be the type of damage hitting the enemies*/);
			}
		}
	}

	uint8 ElementalDamageType = 0;

	MulticastBasicAttackFX(AttackStart, End, bHit, ElementalDamageType);

	GetWorldTimerManager().SetTimer(BasicAttackCooldownTimerHandle, this, &AWizardCharacter::BasicAttackCompleted, BasicAttackSpeed, false);
}

void AWizardCharacter::MulticastBasicAttackFX_Implementation(FVector Start, FVector End, bool bHit, uint8 ElementalDamageType)
{
	switch(ElementalDamageType)
	{
		case 1:
		//lightning light blue
		case 2:
		//fire red orange
		case 3:
		//grass green
		case 4:
		//ice darker blue
		default:
			break;
	}

	DrawDebugCapsule(GetWorld(), Start, BasicAttackHalfHeight, BasicAttackRadius, FQuat::Identity, FColor::Yellow, false, 1.0f);

	DrawDebugCapsule(GetWorld(), End, BasicAttackHalfHeight, BasicAttackRadius, FQuat::Identity, bHit ? FColor::Red : FColor::Green, false, 1.0f);
}

//resets basic attack cd
void AWizardCharacter::BasicAttackCompleted()
{
	bCanBasicAttack = true;
}

// Basic Attack End


// TF right click - cone
//shift is teleport
//r random meteors