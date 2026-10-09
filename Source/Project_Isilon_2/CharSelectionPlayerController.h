// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CharSelectionPlayerController.generated.h"

/**
 * 
 */

class UUserWidget;

UENUM(BlueprintType)
enum class EPlayableCharacter : uint8
{
	Barbarian UMETA(DisplayName = "Barbarian"),
	Wizard UMETA(DisplayName = "Wizard"),
	Ranger UMETA(DisplayName = "Ranger")
};

UCLASS()
class PROJECT_ISILON_2_API ACharSelectionPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;	

	UFUNCTION(BlueprintCallable, Category = "Character Selection")
	void SelectCharacter(EPlayableCharacter PlayerChar);

	UFUNCTION(Server, Reliable)
	void ServerSelectCharacter(EPlayableCharacter PlayerChar);

	UFUNCTION(Server, Reliable)
	void ClientFinishCharacterSelection();

	EPlayableCharacter SelectedCharacter = EPlayableCharacter::Barbarian;

	bool bHasSelectedCharacter = false;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Character Selection")
	TSubclassOf<UUserWidget> CharacterSelectionWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> CharacterSelectionWidget;

	void ShowCharacterSelection();
};
