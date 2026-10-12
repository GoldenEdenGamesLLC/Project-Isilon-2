// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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

UCLASS()
class PROJECT_ISILON_2_API AWizardCharacter : public ACharacter
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
	// Stats
	// ===========================================
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, EditDefaultsOnly, Category = "Defense")
	float CurrentHealth = 108.0f;
	
	float BaseHealth = 108.0f;
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

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Ultimate;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Utility;

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
