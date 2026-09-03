// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "CubeCharacter.generated.h"

class UInputMappingContext;
class AInteractableActor;
class USphereComponent;
class UInputAction;
class UUserWidget;

struct FInputActionInstance;
struct FInputActionValue;

struct FSphereInteractionParams
{
	UPrimitiveComponent* OverlappedComponent = nullptr;
	AActor* OtherActor = nullptr;
	UPrimitiveComponent* OtherComp = nullptr;
	int32 OtherBodyIndex = INDEX_NONE;
	bool bFromSweep = false;
	FHitResult SweepResult;
};

UCLASS()
class PROJECT_ISILON_2_API ACubeCharacter : public ACharacter
{
	GENERATED_BODY()
	
public:
	ACubeCharacter();
	
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;
	virtual void PawnClientRestart() override;

	//Jump overrides
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;
	virtual void Landed(const FHitResult& Hit) override;
	
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
	// Dash
	// ===========================================
	bool bCanDash = true;
	bool bIsDashing = false;
	bool bDashStartedOnGround = false;

	FTimerHandle DashCooldownTimerHandle;
	FTimerHandle DashDurationTimerHandle;

	ECollisionResponse PreviousPawnCollisionResponse = ECR_Block;
	TSet<AActor*> damagedActorsByDash;

	UPROPERTY(EditDefaultsOnly, Category = "Dashing")
	float DashStrength = 4000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dashing")
	float DashCapsuleHalfHeight = 40.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dashing")
	float DashDamageStartRadius = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dashing")
	float DashDamageEndRadius = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dashing")
	float DashDamage = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dashing")
	float DashCooldownTime = 4.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Dashing")
	float DashDuration = 0.20f;
	
	void DashPressed();
	void ResetDashCooldown();
	void StopDash();

	void HandleDashDamage();
	void EnterGhostMode();
	void ExitGhostMode();

	UFUNCTION(Server, Reliable)
	void ServerEnterGhostMode();

	UFUNCTION(Server, Reliable)
	void ServerExitGhostMode();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastDashHandlerFX(FVector Start, FVector End, bool bHit);

	// ===========================================
	// Basic Attack
	// ===========================================
	
	bool bCanBasicAttack = true;
	FTimerHandle BasicAttackCooldownTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	float BasicAttackHalfHeight = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	float BasicAttackRadius = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	float BasicAttackStartDistance = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	float BasicAttackEndDistance = 100.0f;

	void BasicAttackPressed();
	void PerformBasicAttack(FRotator AimRotation);

	UFUNCTION(Server, Reliable)
	void ServerBasicAttack(FRotator AimRotation);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastBasicAttackFX(FVector Start, FVector End, bool bHit);

	void BasicAttackCompleted();
	void ResetBasicAttackCooldown();

	// ===========================================
	// Stats
	// ===========================================
	
	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	float BasicAttackSpeed = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	float BasicAttackDamage = 200.0f;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, EditDefaultsOnly, Category = "Defense")
	float CurrentHealth = 200.0f;
	
	float BaseHealth = 200.0f;
	float MaxHealth = 200.0f;
	
	float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

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
