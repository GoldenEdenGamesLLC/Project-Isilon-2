// Fill out your copyright notice in the Description page of Project Settings.


#include "Heros/WizardCharacter.h"

#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Heros/InteractableActor.h"
#include "Data/GameplayTags.h"
#include "Libraries/GameplayAbilitySystemLibrary.h"
#include "Interfaces/EnemyInterface.h"
#include "Enemies/EnemyAIController.h"
#include "AbilitySystem/TagAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

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
#include "Math/ColorList.h"

// Sets default values
AWizardCharacter::AWizardCharacter()
{
 	bReplicates = true;
	SetReplicateMovement(true);

	ElementalBasicAttackDamageTypes.AddTag(GameplayTags::Combat::DamageTypes::Lightning);
	ElementalBasicAttackDamageTypes.AddTag(GameplayTags::Combat::DamageTypes::Fire);
	ElementalBasicAttackDamageTypes.AddTag(GameplayTags::Combat::DamageTypes::Frost);
	ElementalBasicAttackDamageTypes.AddTag(GameplayTags::Combat::DamageTypes::Celestial);

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

void AWizardCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//don't leave enemies frozen/slowed forever if the wizard goes away
	if(HasAuthority())
	{
		ClearConeExplosionEffects();
	}

	Super::EndPlay(EndPlayReason);
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
		EnhancedInputComponent->BindAction(IA_ControlledChaos, ETriggerEvent::Started, this, &AWizardCharacter::ControlledChaosBlastPressed);
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

	// The base class applies this hit to the GAS Health attribute; HandleHealthChanged mirrors it back
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	return ActualDamage;
}

void AWizardCharacter::HandleHealthChanged(float OldHealth, float NewHealth)
{
	CurrentHealth = NewHealth;
	//UE_LOG(LogTemp, Warning, TEXT("[SERVER] %s took %.1f damage. Health: %.1f"), *GetName(), OldHealth - NewHealth, CurrentHealth);

	if(OldHealth > 0.0f && NewHealth <= 0.0f)
	{
		//die
	}
}

void AWizardCharacter::GetDefaultHealth(float& OutHealth, float& OutMaxHealth) const
{
	OutHealth = CurrentHealth;
	OutMaxHealth = MaxHealth;
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

	//DAMAGE TYPING FOR SPELLS
	UE_LOG(LogTemp, Warning, TEXT("[WIZARD] bElementSelected = %s"), (bElementSelected ? TEXT("TRUE") : TEXT("FALSE")));
	const FGameplayTag DamageType = bElementSelected //used in conjuction with right click
	? 
	UGameplayAbilitySystemLibrary::GetSpecificTagFromContainer(ElementalBasicAttackDamageTypes, ControlledChaosBlastElementalDamageType) 
	: 
	UGameplayAbilitySystemLibrary::GetRandomTagFromContainer(ElementalBasicAttackDamageTypes);

	TSet<AActor*> damagedActors;
	if(bHit)
	{
		for(auto& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if(HitActor)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SERVER] Basic Attack Hit: %s"), *HitActor->GetName());

				bool bAlreadyHit = false;
				damagedActors.Add(HitActor, &bAlreadyHit);

				if(DamageType == GameplayTags::Combat::DamageTypes::Celestial)
				{
					DealDamage(HitActor, BasicAttackDamage * CelestialDamageMultiplicationStrength, DamageType);
				}
				else
				{
					DealDamage(HitActor, BasicAttackDamage, DamageType);
				}

				//elemental stacks, once per enemy per attack
				if(!bAlreadyHit && HitActor->Implements<UEnemyInterface>() && IEnemyInterface::Execute_IsEnemyActive(HitActor))
				{
					AActor* EnemyActor = IEnemyInterface::Execute_GetEnemyActor(HitActor);
					if(IsValid(EnemyActor))
					{
						ApplyElementalStacks(EnemyActor, DamageType);
					}
				}
			}
		}
	}

	ElementalDamageType = bElementSelected ? ControlledChaosBlastElementalDamageType : 0;
	// Matches the cases in MulticastBasicAttackFX
	// probably implement color variance by about 5% - 15% so its not too basic
	if(DamageType == GameplayTags::Combat::DamageTypes::Lightning)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WIZARD BASIC ATTACK] Damage Type = Lightning."));
		
		ElementalDamageType = 0;
	}
	else if(DamageType == GameplayTags::Combat::DamageTypes::Fire)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WIZARD BASIC ATTACK] Damage Type = Fire."));
		ElementalDamageType = 1;
	}
	else if(DamageType == GameplayTags::Combat::DamageTypes::Frost)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WIZARD BASIC ATTACK] Damage Type = Frost."));
		ElementalDamageType = 2;
	}
	else if(DamageType == GameplayTags::Combat::DamageTypes::Celestial)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WIZARD BASIC ATTACK] Damage Type = Celestial."));
		ElementalDamageType = 3;
	}

	MulticastBasicAttackFX(AttackStart, End, bHit);

	GetWorldTimerManager().SetTimer(BasicAttackCooldownTimerHandle, this, &AWizardCharacter::BasicAttackCompleted, BasicAttackSpeed, false);
}

void AWizardCharacter::MulticastBasicAttackFX_Implementation(FVector Start, FVector End, bool bHit)
{
	FColor ElementalType = FColor::White;

	const float ColorVariant = FMath::FRandRange(-2.5f, 2.5f);
	// UE_LOG(LogTemp, Warning, TEXT("Color Variant Percentage : %f"), ColorVariant);

	switch(ElementalDamageType)
	{
		case 0: //lightning
			ElementalType = FColor(39 + ColorVariant, 224 + ColorVariant, 245 + ColorVariant);
			
			break;
		case 1: //fire
			ElementalType = FColor(245 + ColorVariant, 87 + ColorVariant, 39 + ColorVariant);
			
			break;
		case 2: //ice
			ElementalType = FColor(61 + ColorVariant, 169 + ColorVariant, 253 + ColorVariant);
			
			break;
		case 3: //Celestial
			ElementalType = FColor(140 + ColorVariant, 71 + ColorVariant, 67 + ColorVariant);
			
			break;
		default:
			break;
	}

	DrawDebugCapsule(GetWorld(), Start, BasicAttackHalfHeight, BasicAttackRadius, FQuat::Identity, FColor::Yellow, false, 1.0f);

	DrawDebugCapsule(GetWorld(), End, BasicAttackHalfHeight, BasicAttackRadius, FQuat::Identity, ElementalType, false, 1.0f);
}

//resets basic attack cd
void AWizardCharacter::BasicAttackCompleted()
{
	bCanBasicAttack = true;
}

// Basic Attack End

// Start Controlled Chaos Attack

void AWizardCharacter::ControlledChaosBlastPressed()
{
	if(HasAuthority())
	{
		ServerPerformControlledChaosBlast_Implementation();
	}
	else
	{
		ServerPerformControlledChaosBlast();
	}
}

void AWizardCharacter::ServerPerformControlledChaosBlast_Implementation()
{
	if(!bCanControlChaosBlast)
	{
		return;
	}
	
	//picking element
	if(!bElementSelected && bCurrentlySelecting)
	{
		ControlledChaosBlastElementalDamageType = CurrentElement;
		// UE_LOG(LogTemp, Warning, TEXT("[WIZARD] Currently selecting :%d."), ControlledChaosBlastElementalDamageType);
		
		//element has been chosen
		bCanControlChaosBlast = false;
		bElementSelected = true;
		bCurrentlySelecting = false;
		
		//explosion
		UE_LOG(LogTemp, Warning, TEXT("[WIZARD] Firing off explosion."));
		ConeExplosion();
	}

	//wheel
	if(!bElementSelected && !bCurrentlySelecting)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WIZARD] Curently wheeling."));
		bCurrentlySelecting = true;
		SelectingElement();
	}
}

void AWizardCharacter::ConeExplosion()
{
	if(!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(ResetControlledChaosCooldown, this, &AWizardCharacter::ResetControlledChaosBlastCompleted, ControlledChaosBlastCooldown, false);

	//same index lookup as basic attack (index is 1 based)
	const FGameplayTag DamageType = UGameplayAbilitySystemLibrary::GetSpecificTagFromContainer(ElementalBasicAttackDamageTypes, ControlledChaosBlastElementalDamageType);

	//aim with the camera yaw, flat on the ground
	const FRotator ControlRot = Controller ? Controller->GetControlRotation() : GetActorRotation();
	const FRotator YawRotation(0.0f, ControlRot.Yaw, 0.0f);
	const FVector ConeDirection = YawRotation.Vector();
	const FVector Origin = GetActorLocation();

	SetActorRotation(YawRotation);

	//grab everything that could be inside the cone, then filter by inner circle / cone angle
	TArray<FOverlapResult> OverlapResults;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	const FCollisionShape ExplosionSphere = FCollisionShape::MakeSphere(ControlledChaosBlastEndDistance);
	GetWorld()->OverlapMultiByObjectType(OverlapResults, Origin, FQuat::Identity, ObjectParams, ExplosionSphere, QueryParams);

	//enemies can have multiple components overlapping
	TSet<AActor*> HitEnemies;

	for(const FOverlapResult& OverlapHits : OverlapResults)
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
		if(!IsValid(EnemyActor) || HitEnemies.Contains(EnemyActor))
		{
			continue;
		}

		if(!IsInConeExplosion(Origin, ConeDirection, EnemyActor->GetActorLocation()))
		{
			continue;
		}

		HitEnemies.Add(EnemyActor);
		UE_LOG(LogTemp, Warning, TEXT("[WIZARD CONE EXPLOSION] Hit %s"), *EnemyActor->GetName());

		DealDamage(EnemyActor, ConeExplosionDamage, DamageType);
		ApplyConeExplosionEffect(EnemyActor, DamageType);
	}

	FColor ElementColor = FColor::White;
	if(DamageType == GameplayTags::Combat::DamageTypes::Lightning)
	{
		ElementColor = FColor(39, 224, 245);
	}
	else if(DamageType == GameplayTags::Combat::DamageTypes::Fire)
	{
		ElementColor = FColor(245, 87, 39);
	}
	else if(DamageType == GameplayTags::Combat::DamageTypes::Frost)
	{
		ElementColor = FColor(61, 169, 253);
	}
	else if(DamageType == GameplayTags::Combat::DamageTypes::Celestial)
	{
		ElementColor = FColor(140, 71, 67);
	}

	MulticastConeExplosionFX(Origin, ConeDirection, ElementColor);
}

bool AWizardCharacter::IsInConeExplosion(const FVector& Origin, const FVector& ConeDirection, const FVector& EnemyLocation) const
{
	//flatten so enemies above/below still count
	FVector ToEnemy = EnemyLocation - Origin;
	ToEnemy.Z = 0.0f;

	const float DistanceSquared = ToEnemy.SizeSquared();

	//inner circle where the cone starts
	if(DistanceSquared <= FMath::Square(ConeExplosionInnerRadius))
	{
		return true;
	}

	if(DistanceSquared > FMath::Square(ControlledChaosBlastEndDistance))
	{
		return false;
	}

	//cone: angle between aim and enemy must be within half the cone width
	const float HalfAngleCos = FMath::Cos(FMath::DegreesToRadians(ConeExplosionAngle * 0.5f));
	const float Dot = FVector::DotProduct(ConeDirection.GetSafeNormal2D(), ToEnemy.GetSafeNormal());

	return Dot >= HalfAngleCos;
}

void AWizardCharacter::ApplyConeExplosionEffect(AActor* EnemyActor, const FGameplayTag& DamageType)
{
	if(DamageType == GameplayTags::Combat::DamageTypes::Celestial)
	{
		DealMultipliedDamage(EnemyActor);
		return;
	}

	ApplyElementalStacks(EnemyActor, DamageType);
}

// shared by basic attack and cone explosion
void AWizardCharacter::ApplyElementalStacks(AActor* EnemyActor, const FGameplayTag& DamageType)
{
	if(DamageType == GameplayTags::Combat::DamageTypes::Frost)
	{
		ApplyChill(Cast<ACharacter>(EnemyActor));
	}
	else if(DamageType == GameplayTags::Combat::DamageTypes::Lightning)
	{
		ApplyShock(Cast<ACharacter>(EnemyActor));
	}
	else if(DamageType == GameplayTags::Combat::DamageTypes::Fire)
	{
		ApplyBurn(EnemyActor);
	}
}

// Lightning - stacks, on max stacks deal damage and stun
void AWizardCharacter::ApplyShock(ACharacter* EnemyCharacter)
{
	if(!IsValid(EnemyCharacter))
	{
		return;
	}

	//no building stacks while already stunned
	if(StunnedEnemies.Contains(EnemyCharacter))
	{
		return;
	}

	FWizardShockData& ShockData = ShockedEnemies.FindOrAdd(EnemyCharacter);
	ShockData.Stacks++;

	UE_LOG(LogTemp, Warning, TEXT("[WIZARD LIGHTNING] %s stacks = %d"), *EnemyCharacter->GetName(), ShockData.Stacks);

	if(ShockData.Stacks >= MaxLightningStacks)
	{
		GetWorldTimerManager().ClearTimer(ShockData.StackTimerHandle);
		ShockedEnemies.Remove(EnemyCharacter);

		DealDamage(EnemyCharacter, LightningOverloadDamage, GameplayTags::Combat::DamageTypes::Lightning);

		//overload damage might have killed it
		if(IEnemyInterface::Execute_IsEnemyActive(EnemyCharacter))
		{
			StunEnemy(EnemyCharacter);
		}
		return;
	}

	//refresh stack duration
	const FTimerDelegate ExpireDelegate = FTimerDelegate::CreateUObject(this, &AWizardCharacter::ExpireShockStacks, TWeakObjectPtr<ACharacter>(EnemyCharacter));
	GetWorldTimerManager().SetTimer(ShockData.StackTimerHandle, ExpireDelegate, LightningStackDuration, false);
}

void AWizardCharacter::ExpireShockStacks(TWeakObjectPtr<ACharacter> EnemyCharacter)
{
	ShockedEnemies.Remove(EnemyCharacter);
}

void AWizardCharacter::StunEnemy(ACharacter* EnemyCharacter)
{
	FTimerHandle& StunHandle = StunnedEnemies.FindOrAdd(EnemyCharacter);

	if(AEnemyAIController* AIController = Cast<AEnemyAIController>(EnemyCharacter->GetController()))
	{
		AIController->SetCrowdControlActive(true);
	}

	if(UCharacterMovementComponent* Movement = EnemyCharacter->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	const FTimerDelegate EndStunDelegate = FTimerDelegate::CreateUObject(this, &AWizardCharacter::EndStun, TWeakObjectPtr<ACharacter>(EnemyCharacter));
	GetWorldTimerManager().SetTimer(StunHandle, EndStunDelegate, StunDuration, false);
}

void AWizardCharacter::EndStun(TWeakObjectPtr<ACharacter> EnemyCharacter)
{
	StunnedEnemies.Remove(EnemyCharacter);

	if(!EnemyCharacter.IsValid())
	{
		return;
	}

	if(UCharacterMovementComponent* Movement = EnemyCharacter->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}

	if(AEnemyAIController* AIController = Cast<AEnemyAIController>(EnemyCharacter->GetController()))
	{
		AIController->SetCrowdControlActive(false);
	}
}

// Frost - each stack slows more, on max stacks root
void AWizardCharacter::ApplyChill(ACharacter* EnemyCharacter)
{
	if(!IsValid(EnemyCharacter))
	{
		return;
	}

	//already rooted, its walk speed is 0 so don't start stacking off of that
	if(RootedEnemies.Contains(EnemyCharacter))
	{
		return;
	}

	UCharacterMovementComponent* Movement = EnemyCharacter->GetCharacterMovement();
	if(!Movement)
	{
		return;
	}

	//store the original speed on the first stack only
	FWizardChillData* ChillData = ChilledEnemies.Find(EnemyCharacter);
	if(!ChillData)
	{
		ChillData = &ChilledEnemies.Add(EnemyCharacter);
		ChillData->OriginalMaxWalkSpeed = Movement->MaxWalkSpeed;
	}

	ChillData->Stacks++;

	UE_LOG(LogTemp, Warning, TEXT("[WIZARD FROST] %s stacks = %d"), *EnemyCharacter->GetName(), ChillData->Stacks);

	if(ChillData->Stacks >= MaxFrostStacks)
	{
		const float OriginalSpeed = ChillData->OriginalMaxWalkSpeed;

		GetWorldTimerManager().ClearTimer(ChillData->StackTimerHandle);
		ChilledEnemies.Remove(EnemyCharacter);

		RootEnemy(EnemyCharacter, OriginalSpeed);
		return;
	}

	const float SlowPercent = FMath::Min(ChillData->Stacks * FrostSlowPerStack, FrostMaxSlow);
	Movement->MaxWalkSpeed = ChillData->OriginalMaxWalkSpeed * (1.0f - SlowPercent);

	//refresh stack duration
	const FTimerDelegate ExpireDelegate = FTimerDelegate::CreateUObject(this, &AWizardCharacter::ExpireChillStacks, TWeakObjectPtr<ACharacter>(EnemyCharacter));
	GetWorldTimerManager().SetTimer(ChillData->StackTimerHandle, ExpireDelegate, FrostStackDuration, false);
}

void AWizardCharacter::ExpireChillStacks(TWeakObjectPtr<ACharacter> EnemyCharacter)
{
	FWizardChillData ChillData;
	if(!ChilledEnemies.RemoveAndCopyValue(EnemyCharacter, ChillData))
	{
		return;
	}

	if(!EnemyCharacter.IsValid())
	{
		return;
	}

	if(UCharacterMovementComponent* Movement = EnemyCharacter->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = ChillData.OriginalMaxWalkSpeed;
	}
}

void AWizardCharacter::RootEnemy(ACharacter* EnemyCharacter, float OriginalMaxWalkSpeed)
{
	UE_LOG(LogTemp, Warning, TEXT("[WIZARD FROST] Rooted %s"), *EnemyCharacter->GetName());

	//root = can't move but can still attack if in range (stun stops both)
	if(UCharacterMovementComponent* Movement = EnemyCharacter->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->MaxWalkSpeed = 0.0f;
	}

	FWizardRootData& RootData = RootedEnemies.FindOrAdd(EnemyCharacter);
	RootData.OriginalMaxWalkSpeed = OriginalMaxWalkSpeed;

	const FTimerDelegate EndRootDelegate = FTimerDelegate::CreateUObject(this, &AWizardCharacter::EndRoot, TWeakObjectPtr<ACharacter>(EnemyCharacter));
	GetWorldTimerManager().SetTimer(RootData.TimerHandle, EndRootDelegate, RootDuration, false);
}

void AWizardCharacter::EndRoot(TWeakObjectPtr<ACharacter> EnemyCharacter)
{
	FWizardRootData RootData;
	if(!RootedEnemies.RemoveAndCopyValue(EnemyCharacter, RootData))
	{
		return;
	}

	if(!EnemyCharacter.IsValid())
	{
		return;
	}

	if(UCharacterMovementComponent* Movement = EnemyCharacter->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = RootData.OriginalMaxWalkSpeed;
	}
}

// Fire - stacking burn
void AWizardCharacter::ApplyBurn(AActor* EnemyActor)
{
	if(!IsValid(EnemyActor))
	{
		return;
	}

	//every application adds a stack and refreshes the duration
	FWizardBurnData& BurnData = BurningEnemies.FindOrAdd(EnemyActor);
	BurnData.Stacks = FMath::Min(BurnData.Stacks + 1, MaxBurnStacks);
	BurnData.TicksRemaining = BurnTickCount;

	UE_LOG(LogTemp, Warning, TEXT("[WIZARD FIRE] %s stacks = %d"), *EnemyActor->GetName(), BurnData.Stacks);

	if(!GetWorldTimerManager().IsTimerActive(BurnData.TimerHandle))
	{
		const FTimerDelegate BurnDelegate = FTimerDelegate::CreateUObject(this, &AWizardCharacter::TickBurn, TWeakObjectPtr<AActor>(EnemyActor));
		GetWorldTimerManager().SetTimer(BurnData.TimerHandle, BurnDelegate, BurnTickInterval, true);
	}
}

void AWizardCharacter::TickBurn(TWeakObjectPtr<AActor> EnemyActor)
{
	FWizardBurnData* BurnData = BurningEnemies.Find(EnemyActor);
	if(!BurnData)
	{
		return;
	}

	//enemy died / went back to the pool
	const bool bEnemyActive = EnemyActor.IsValid()
		&& EnemyActor->Implements<UEnemyInterface>()
		&& IEnemyInterface::Execute_IsEnemyActive(EnemyActor.Get());

	//burn ran out, stacks are lost
	if(!bEnemyActive || BurnData->TicksRemaining <= 0)
	{
		GetWorldTimerManager().ClearTimer(BurnData->TimerHandle);
		BurningEnemies.Remove(EnemyActor);
		return;
	}

	BurnData->TicksRemaining--;
	DealDamage(EnemyActor.Get(), BurnDamagePerStack * BurnData->Stacks, GameplayTags::Combat::DamageTypes::Fire);
}

// Celestial - Deal Multiplied Damage
void AWizardCharacter::DealMultipliedDamage(AActor* EnemyActor)
{
	UAbilitySystemComponent* EnemyASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(EnemyActor);
	if(!EnemyASC)
	{
		return;
	}

	const float Health = EnemyASC->GetNumericAttribute(UTagAttributeSet::GetHealthAttribute());
	const float EnemyMaxHealth = EnemyASC->GetNumericAttribute(UTagAttributeSet::GetMaxHealthAttribute());

	//already dead from the explosion damage
	if(Health <= 0.0f || EnemyMaxHealth <= 0.0f)
	{
		return;
	}

	//deals damage once again
	DealDamage(EnemyActor, ConeExplosionDamage,GameplayTags::Combat::DamageTypes::Celestial);
}

void AWizardCharacter::ClearConeExplosionEffects()
{
	TArray<TWeakObjectPtr<ACharacter>> StunnedKeys;
	StunnedEnemies.GetKeys(StunnedKeys);
	for(const TWeakObjectPtr<ACharacter>& Enemy : StunnedKeys)
	{
		GetWorldTimerManager().ClearTimer(StunnedEnemies[Enemy]);
		EndStun(Enemy);
	}

	TArray<TWeakObjectPtr<ACharacter>> RootedKeys;
	RootedEnemies.GetKeys(RootedKeys);
	for(const TWeakObjectPtr<ACharacter>& Enemy : RootedKeys)
	{
		GetWorldTimerManager().ClearTimer(RootedEnemies[Enemy].TimerHandle);
		EndRoot(Enemy);
	}

	TArray<TWeakObjectPtr<ACharacter>> ChilledKeys;
	ChilledEnemies.GetKeys(ChilledKeys);
	for(const TWeakObjectPtr<ACharacter>& Enemy : ChilledKeys)
	{
		GetWorldTimerManager().ClearTimer(ChilledEnemies[Enemy].StackTimerHandle);
		ExpireChillStacks(Enemy);
	}

	for(TPair<TWeakObjectPtr<ACharacter>, FWizardShockData>& Shock : ShockedEnemies)
	{
		GetWorldTimerManager().ClearTimer(Shock.Value.StackTimerHandle);
	}
	ShockedEnemies.Empty();

	for(TPair<TWeakObjectPtr<AActor>, FWizardBurnData>& Burn : BurningEnemies)
	{
		GetWorldTimerManager().ClearTimer(Burn.Value.TimerHandle);
	}
	BurningEnemies.Empty();
}

void AWizardCharacter::MulticastConeExplosionFX_Implementation(FVector Origin, FVector ConeDirection, FColor ElementColor)
{
	DrawDebugCircle(GetWorld(), Origin, ConeExplosionInnerRadius, 32, FColor::Yellow, false, 1.0f, 0, 2.0f, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), false);

	const float HalfAngleRadians = FMath::DegreesToRadians(ConeExplosionAngle * 0.75f);
	DrawDebugCone(GetWorld(), Origin, ConeDirection, ControlledChaosBlastEndDistance, HalfAngleRadians, FMath::DegreesToRadians(15.0f), 16, ElementColor, false, 1.0f);
}

void AWizardCharacter::SelectingElement()
{
	if(bCurrentlySelecting)
	{
		if(CurrentElement > 3)
		{
			CurrentElement = 0;
		}
	
		UE_LOG(LogTemp, Warning, TEXT("[WIZARD] ELEMENT = %d."), CurrentElement);

		GetWorldTimerManager().SetTimer(ControlledChaosChooseElementTimerHandle, this, &AWizardCharacter::SelectingElement, ControlledChaosBlastNextElementTime, true);
		CurrentElement++;
	}
}

void AWizardCharacter::ResetControlledChaosBlastCompleted()
{
	bCanControlChaosBlast = true;
	
	bCurrentlySelecting = false;
	bElementSelected = false;

	GetWorldTimerManager().ClearTimer(ResetControlledChaosCooldown);
	GetWorldTimerManager().ClearTimer(ControlledChaosChooseElementTimerHandle);
}

// End Controlled Chaos Attack

//shift is teleport
//r random meteors