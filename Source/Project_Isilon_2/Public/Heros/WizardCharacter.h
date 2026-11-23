// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Heros/HeroCharacterBase.h"
#include "Logging/LogMacros.h"
#include "InteractionParams.h"
#include "WizardCharacter.generated.h"

class UInputMappingContext;
class AInteractableActor;
class USphereComponent;
class UInputAction;
class UUserWidget;
class ACharacter;

struct FInputActionInstance;
struct FInputActionValue;

// Per-enemy state for Cone Explosion elemental effects
struct FWizardBurnData
{
	int32 Stacks = 0;
	int32 TicksRemaining = 0;
	FTimerHandle TimerHandle;
};

struct FWizardShockData
{
	int32 Stacks = 0;
	FTimerHandle StackTimerHandle;
};

struct FWizardChillData
{
	int32 Stacks = 0;
	float OriginalMaxWalkSpeed = 0.0f;
	FTimerHandle StackTimerHandle;
};

struct FWizardRootData
{
	float OriginalMaxWalkSpeed = 0.0f;
	FTimerHandle TimerHandle;
};

UCLASS()
class PROJECT_ISILON_2_API AWizardCharacter : public AHeroCharacterBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AWizardCharacter();
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PawnClientRestart() override;

	//Jump overrides
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;
	virtual void Landed(const FHitResult& Hit) override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//replicated overrides
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// ===========================================
	// Jump
	// ===========================================

	int32 JumpTotals = 0;
	bool bCanJumpCooldown = true;
	FTimerHandle JumpCooldownTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Jumping")
	float JumpCooldownTime = 0.15f;
	
	void JumpPressed();
	void ResetJumpCooldown();

	// ===========================================
	// Wizard Chaos Bolt
	// ===========================================
	
	uint8 ElementalDamageType = 0;

	bool bCanBasicAttack = true;
	FTimerHandle BasicAttackCooldownTimerHandle;
	
	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	// float BasicAttackSpeed = 1.0f;
	float BasicAttackSpeed = 0.33f;

	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	float BasicAttackDamage = 27.5f; //base damage

	// Chaos damage: each basic attack picks one of these damage types at random
	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack", meta = (Categories = "Combat.DamageType"))
	FGameplayTagContainer ElementalBasicAttackDamageTypes;

	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	float BasicAttackHalfHeight = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	float BasicAttackRadius = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	float BasicAttackStartDistance = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	float BasicAttackEndDistance = 2000.0f;

	void BasicAttackPressed();
	void PerformBasicAttack(FRotator AimRotation);

	UFUNCTION(Server, Reliable)
	void ServerBasicAttack(FRotator AimRotation);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastBasicAttackFX(FVector Start, FVector End, bool bHit);

	void BasicAttackCompleted();
	
	// ===========================================
	// Wizard Controlled Chaos Blast
	// ===========================================

	bool bCanControlChaosBlast = true;
	
	bool bCurrentlySelecting = false;
	bool bElementSelected = false;
	uint8 CurrentElement = 0;
	float ControlledChaosBlastNextElementTime = 0.55f;

	FTimerHandle ResetControlledChaosCooldown;
	FTimerHandle ControlledChaosChooseElementTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Controlled Chaos Blast")
	uint8 ControlledChaosBlastElementalDamageType = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Controlled Chaos Blast")
	float ControlledChaosBlastCooldown = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Controlled Chaos Blast")
	float ControlledChaosBlastEndDistance = 600.0f;

	void ControlledChaosBlastPressed();

	void SelectingElement();

	//UFUNCTION(Server, Reliable)
	void ConeExplosion();

	UFUNCTION(Server, Reliable)
	void ServerPerformControlledChaosBlast();

	void ResetControlledChaosBlastCompleted();

	// ===========================================
	// Wizard Cone Explosion
	// ===========================================

	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion")
	float ConeExplosionDamage = 30.0f;

	// circle around the wizard where the cone starts, hits enemies hugging you
	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion")
	float ConeExplosionInnerRadius = 150.0f;

	// full width of the cone in degrees
	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion", meta = (ClampMin = "1.0", ClampMax = "180.0"))
	float ConeExplosionAngle = 60.0f;

	// Frost - each stack slows more, at max stacks the target is rooted
	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Frost", meta = (ClampMin = "1"))
	int32 MaxFrostStacks = 20;

	// move speed lost per stack (0.04 = 4% per stack)
	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Frost", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FrostSlowPerStack = 0.04f;

	// slow never goes past this before the root kicks in
	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Frost", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FrostMaxSlow = 0.9f;

	// stacks fall off if not reapplied within this time
	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Frost")
	float FrostStackDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Frost")
	float RootDuration = 2.0f;

	// Lightning - builds stacks, on max stacks deals damage and stuns
	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Lightning", meta = (ClampMin = "1"))
	int32 MaxLightningStacks = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Lightning")
	float LightningStackDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Lightning")
	float LightningOverloadDamage = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Lightning")
	float StunDuration = 1.5f;

	// Fire - each application adds a stack, burn damage per tick = stacks * damage per stack
	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Fire", meta = (ClampMin = "1", ClampMax = "255"))
	int32 MaxBurnStacks = 255;

	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Fire")
	float BurnDamagePerStack = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Fire")
	float BurnTickInterval = 0.5f;

	// reapplying refreshes the duration back to this many ticks
	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Fire")
	int32 BurnTickCount = 6;

	// Celestial
	UPROPERTY(EditDefaultsOnly, Category = "Cone Explosion|Celestial", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CelestialDamageMultiplicationStrength = 2.0f;

	TMap<TWeakObjectPtr<AActor>, FWizardBurnData> BurningEnemies;
	TMap<TWeakObjectPtr<ACharacter>, FWizardShockData> ShockedEnemies;
	TMap<TWeakObjectPtr<ACharacter>, FTimerHandle> StunnedEnemies;
	TMap<TWeakObjectPtr<ACharacter>, FWizardChillData> ChilledEnemies;
	TMap<TWeakObjectPtr<ACharacter>, FWizardRootData> RootedEnemies;

	bool IsInConeExplosion(const FVector& Origin, const FVector& ConeDirection, const FVector& EnemyLocation) const;
	void ApplyConeExplosionEffect(AActor* EnemyActor, const FGameplayTag& DamageType);
	void ApplyElementalStacks(AActor* EnemyActor, const FGameplayTag& DamageType);

	// Lightning
	void ApplyShock(ACharacter* EnemyCharacter);
	void ExpireShockStacks(TWeakObjectPtr<ACharacter> EnemyCharacter);
	void StunEnemy(ACharacter* EnemyCharacter);
	void EndStun(TWeakObjectPtr<ACharacter> EnemyCharacter);

	// Frost
	void ApplyChill(ACharacter* EnemyCharacter);
	void ExpireChillStacks(TWeakObjectPtr<ACharacter> EnemyCharacter);
	void RootEnemy(ACharacter* EnemyCharacter, float OriginalMaxWalkSpeed);
	void EndRoot(TWeakObjectPtr<ACharacter> EnemyCharacter);

	// Fire
	void ApplyBurn(AActor* EnemyActor);
	void TickBurn(TWeakObjectPtr<AActor> EnemyActor);

	void DealMultipliedDamage(AActor* EnemyActor);

	void ClearConeExplosionEffects();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastConeExplosionFX(FVector Origin, FVector ConeDirection, FColor ElementColor);

	// ===========================================
	// Stats
	// ===========================================
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, EditDefaultsOnly, Category = "Defense")
	float CurrentHealth = 96.0f;
	
	float BaseHealth = 96.0f;
	float MaxHealth = 96.0f;
	
	float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	virtual void HandleHealthChanged(float OldHealth, float NewHealth) override;
	virtual void GetDefaultHealth(float& OutHealth, float& OutMaxHealth) const override;

	UFUNCTION()
	void OnRep_CurrentHealth();

	// ===========================================
	// Inputs
	// ===========================================
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Dash;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_BasicAttack;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Interact;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Ultimate;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_ControlledChaos;

	void Move(const FInputActionValue& val);
	void Look(const FInputActionValue& val);

	// ===========================================
	// Interaction
	// ===========================================

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY()
	TObjectPtr<AInteractableActor> NearbyInteractable = nullptr;

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void HandleInteractionOverlap(const FSphereInteractionParams& Params);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void InteractWithObject(const FInputActionValue& val);

	UFUNCTION(Server, Reliable)
	void ServerInteract(AInteractableActor* Interactable);

	// ===========================================
	// Camera
	// ===========================================

	UPROPERTY(VisibleAnywhere, Category=Camera)
	class USpringArmComponent* cameraBoom;

	UPROPERTY(VisibleAnywhere, Category=Camera)
	class UCameraComponent* followCamera;

	// ===========================================
	// Crosshairs
	// ===========================================

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> CrosshairWidget;
};
